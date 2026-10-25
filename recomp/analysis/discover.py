"""Function and switch-table discovery by recursive descent over an x86 Image.

The pass does not trust any external function list. Seeds are the entry point,
direct call targets, code pointers found in data sections and code addresses
used as immediates; low-confidence seeds must decode cleanly and must not
overlap committed code. MSVC jump tables (`jmp [idx*4+table]`, optionally
behind a `movzx idx, byte [reg+index_table]`) are resolved from the bounding
`cmp`/`and` and their bytes are classified as data. Functions that can never
return are found by fixpoint so that the bytes after calls to them are not
decoded as code.
"""
from __future__ import annotations

import struct
import sys
from array import array
from collections import Counter, deque
from dataclasses import dataclass, field

from .image import ROOT, Image

sys.path.insert(0, str(ROOT / "local/python-deps"))
from capstone import (CS_ARCH_X86, CS_MODE_32, CS_GRP_JUMP, Cs)  # noqa: E402
from capstone import x86_const as X  # noqa: E402

NORETURN_IMPORTS = frozenset({
    "ExitProcess", "TerminateProcess", "ExitThread", "FatalAppExitA", "RaiseException",
})

# Instruction kinds.
OTHER, RET, STOP, JMP, JMP_IND, JCC, CALL, CALL_IND = range(8)
KIND_NAMES = ("other", "ret", "stop", "jmp", "jmp_ind", "jcc", "call", "call_ind")

CANONICAL = {}
for full, parts in {
    "eax": ("al", "ah", "ax"), "ebx": ("bl", "bh", "bx"), "ecx": ("cl", "ch", "cx"),
    "edx": ("dl", "dh", "dx"), "esi": ("si",), "edi": ("di",), "ebp": ("bp",), "esp": ("sp",),
}.items():
    CANONICAL[full] = full
    for part in parts:
        CANONICAL[part] = full

MAX_TABLE_ENTRIES = 4096
PAD_BYTES = frozenset((0xCC, 0x90, 0x00))
HEURISTIC_REACH = 0x4000  # unbounded tables only accept targets this close to the site
SOURCE_PRIORITY = ("entry", "call", "imm", "data_ptr", "orphan")


@dataclass(slots=True)
class Insn:
    address: int
    size: int
    kind: int
    target: int | None = None          # direct branch/call target
    mem: tuple | None = None           # (base, index, scale, disp) for indirect jmp/call
    import_name: str | None = None     # resolved IAT slot for indirect jmp/call/mov
    code_refs: tuple = ()              # immediates that point into executable sections


@dataclass
class Switch:
    site: int
    table: int
    entries: int
    targets: list[int]
    index_table: int | None
    index_entries: int
    bound: str  # "cmp", "and", "flow:<reg>", "heuristic" or "pending"
    pending: tuple | None = None  # (register, cmp address, inclusive) for register bounds
    index_reg: str = ""           # index register of a heuristic table, for CFG bounds


@dataclass
class Function:
    entry: int
    source: str
    insns: set[int] = field(default_factory=set)
    calls: set[int] = field(default_factory=set)
    tailcalls: set[int] = field(default_factory=set)
    import_calls: set[str] = field(default_factory=set)
    code_refs: set[int] = field(default_factory=set)
    switches: list[Switch] = field(default_factory=list)
    unresolved_jumps: list[int] = field(default_factory=list)
    dynamic_tailcalls: list[int] = field(default_factory=list)
    invalid: list[int] = field(default_factory=list)
    into_data: list[int] = field(default_factory=list)  # paths cut at a known table
    has_ret: bool = False
    returns_via_tail: bool = False


class Discovery:
    def __init__(self, image: Image, noreturn_imports=NORETURN_IMPORTS):
        self.image = image
        self.noreturn_imports = set(noreturn_imports)
        self.md = Cs(CS_ARCH_X86, CS_MODE_32)
        self.md.detail = True
        self.cache: dict[int, Insn | None] = {}
        self.end_map: dict[int, int] = {}
        self.functions: dict[int, Function] = {}
        self.rejected: dict[int, str] = {}
        self.noreturn: set[int] = set()
        exec_sections = image.exec_sections
        self.code_lo = min(s.start for s in exec_sections)
        self.code_hi = max(s.end for s in exec_sections)
        # Bytes known to be jump/index tables; exploration never decodes through them.
        self.data_mask = bytearray(self.code_hi - self.code_lo)
        self._scope: set[int] | None = None  # instructions of the function being explored
        self._last_const = 0
        self._last_copy = ("", 0)

    # -- decoding -----------------------------------------------------------------
    def _decode_detail(self, va: int):
        data = self.image.read(va, 16)
        for ins in self.md.disasm(data, va, 1):
            return ins
        return None

    def decode(self, va: int) -> Insn | None:
        if va in self.cache:
            return self.cache[va]
        insn = None
        if self.image.is_exec(va):
            ins = self._decode_detail(va)
            if ins is not None:
                insn = self._classify(ins)
        self.cache[va] = insn
        if insn is not None:
            self.end_map.setdefault(va + insn.size, []).append(va)
        return insn

    def _in_code(self, value: int) -> bool:
        return self.code_lo <= value < self.code_hi and self.image.is_exec(value)

    def _classify(self, ins) -> Insn:
        iid = ins.id
        ops = ins.operands
        insn = Insn(ins.address, ins.size, OTHER)
        if iid in (X.X86_INS_RET, X.X86_INS_RETF):
            insn.kind = RET
        elif iid in (X.X86_INS_INT3, X.X86_INS_HLT, X.X86_INS_UD2):
            insn.kind = STOP
        elif iid in (X.X86_INS_JMP, X.X86_INS_CALL):
            direct = iid == X.X86_INS_JMP and JMP or CALL
            op = ops[0] if ops else None
            if op is not None and op.type == X.X86_OP_IMM:
                insn.kind = direct
                insn.target = op.imm & 0xFFFFFFFF
            else:
                insn.kind = JMP_IND if iid == X.X86_INS_JMP else CALL_IND
                if op is not None and op.type == X.X86_OP_MEM:
                    m = op.mem
                    disp = m.disp & 0xFFFFFFFF
                    insn.mem = (m.base, m.index, m.scale, disp)
                    if m.base == 0 and m.index == 0 and disp in self.image.imports:
                        insn.import_name = self.image.imports[disp][1]
        elif CS_GRP_JUMP in ins.groups:
            insn.kind = JCC
            insn.target = ops[0].imm & 0xFFFFFFFF
        refs = []
        if insn.kind in (OTHER,):
            for op in ops:
                if op.type == X.X86_OP_IMM:
                    value = op.imm & 0xFFFFFFFF
                    if self._in_code(value):
                        refs.append(value)
                elif op.type == X.X86_OP_MEM and op.mem.base == 0 and op.mem.index == 0:
                    disp = op.mem.disp & 0xFFFFFFFF
                    if disp in self.image.imports:
                        insn.import_name = self.image.imports[disp][1]
        insn.code_refs = tuple(refs)
        return insn

    def prev_insn(self, va: int) -> int | None:
        """Instruction ending at va, preferring one of the function being explored."""
        candidates = self.end_map.get(va)
        if not candidates:
            return None
        if self._scope is not None:
            for start in candidates:
                if start in self._scope:
                    return start
        return candidates[0]

    def _mark_data(self, start: int, size: int) -> None:
        lo = max(start - self.code_lo, 0)
        hi = min(start + size - self.code_lo, len(self.data_mask))
        for b in range(lo, hi):
            self.data_mask[b] = 1

    def _hits_data(self, va: int, size: int) -> bool:
        lo = va - self.code_lo
        return any(self.data_mask[lo:lo + size])

    # -- switch tables --------------------------------------------------------------
    def _reg(self, ins, reg_id) -> str:
        return CANONICAL.get(ins.reg_name(reg_id), ins.reg_name(reg_id))

    def _recent_immediate(self, va: int, reg: str, window: int = 400) -> int | None:
        """Constant last loaded into reg (`mov reg, imm` / `xor reg, reg`) before va.

        Walks the linear predecessor chain of the function being explored. MSVC keeps
        switch bounds in callee-saved registers (ebx/ebp/esi/edi) set near the
        prologue, so the walk may cross calls, which only clobber eax/ecx/edx.
        """
        for _ in range(window):
            prev = self.prev_insn(va)
            if prev is None:
                return None
            d = self._decode_detail(prev)
            va = prev
            if d is None:
                return None
            if d.id == X.X86_INS_CALL and reg in ("eax", "ecx", "edx"):
                return None
            if not d.operands:
                continue
            op0 = d.operands[0]
            if op0.type == X.X86_OP_REG and self._reg(d, op0.reg) == reg:
                if d.id == X.X86_INS_MOV and d.operands[1].type == X.X86_OP_IMM:
                    return d.operands[1].imm & 0xFFFFFFFF
                if (d.id == X.X86_INS_XOR and d.operands[1].type == X.X86_OP_REG
                        and self._reg(d, d.operands[1].reg) == reg):
                    return 0
                if d.id in (X.X86_INS_CMP, X.X86_INS_TEST, X.X86_INS_PUSH):
                    continue  # reads only
                return None
        return None

    def _table_targets(self, site: int, table: int, count: int, index_table: int | None):
        """(table, entries, targets, index_entries) for a bounded table, or None."""
        if not 0 < count <= MAX_TABLE_ENTRIES:
            return None
        index_entries = 0
        if index_table is not None:
            raw = self.image.read(index_table, count)
            if len(raw) != count:
                return None
            index_entries = count
            count = max(raw) + 1
        targets = []
        for i in range(count):
            value = self.image.u32(table + 4 * i)
            if value is None or not self._in_code(value):
                if i == 0 and count > 1:
                    # MSVC biases tables whose index is known to be >= 1
                    # (`jmp [idx*4 + vec - 4]`): slot 0 is never read.
                    continue
                return None
            targets.append(value)
        if len(targets) < count:
            table += 4
            count -= 1
        return table, count, targets, index_entries

    def resolve_switch(self, site: Insn) -> Switch | None:
        if site.mem is None:
            return None
        base, index, scale, table = site.mem
        if base != 0 or index == 0 or scale != 4:
            return None
        site_ins = self._decode_detail(site.address)
        tracked = self._reg(site_ins, index)
        count = None
        bound = "heuristic"
        index_table = None
        pending = None
        va = site.address
        for _ in range(24):
            prev = self.prev_insn(va)
            if prev is None:
                break
            d = self._decode_detail(prev)
            va = prev
            if d is None or not d.operands:
                continue
            op0 = d.operands[0]
            writes_tracked = op0.type == X.X86_OP_REG and self._reg(d, op0.reg) == tracked
            if d.id == X.X86_INS_MOVZX and writes_tracked and d.operands[1].type == X.X86_OP_MEM:
                m = d.operands[1].mem
                if d.operands[1].size == 1 and m.base != 0 and m.index == 0:
                    index_table = m.disp & 0xFFFFFFFF
                    tracked = self._reg(d, m.base)
                    continue
            if d.id == X.X86_INS_CMP and writes_tracked and d.operands[1].type in (X.X86_OP_IMM, X.X86_OP_REG):
                after = self._decode_detail(d.address + d.size)
                inclusive = not (after is not None and after.mnemonic in ("jae", "jb", "jnb", "jnae"))
                if d.operands[1].type == X.X86_OP_IMM:
                    imm = d.operands[1].imm & 0xFFFFFFFF
                else:  # cmp idx, reg where reg holds a constant
                    other = self._reg(d, d.operands[1].reg)
                    imm = self._recent_immediate(d.address, other)
                    if imm is None:
                        # Resolved on the complete body by _resolve_pending.
                        pending = (other, d.address, inclusive)
                        break
                count = imm + 1 if inclusive else imm
                bound = "cmp"
                break
            if d.id == X.X86_INS_AND and writes_tracked and d.operands[1].type == X.X86_OP_IMM:
                count = (d.operands[1].imm & 0xFFFFFFFF) + 1
                bound = "and"
                break
            if d.id == X.X86_INS_MOV and writes_tracked:
                if d.operands[1].type == X.X86_OP_REG:
                    tracked = self._reg(d, d.operands[1].reg)
                    continue
                break  # loaded from memory: the bound is not visible here
            if d.id in (X.X86_INS_CALL,) or (d.id == X.X86_INS_JMP):
                break
        if count is not None:
            resolved = self._table_targets(site.address, table, count, index_table)
            if resolved is None:
                return None
            table, count, targets, index_entries = resolved
            return Switch(site.address, table, count, targets, index_table, index_entries, bound)
        if pending is not None and index_table is not None:
            # Two-level table with a register bound: nothing to follow until resolved.
            return Switch(site.address, table, 0, [], index_table, 0, "pending", pending)
        # Unbounded: read entries while they point near the dispatch site and the
        # table does not run into code already decoded for this function.
        targets = []
        for i in range(256):
            value = self.image.u32(table + 4 * i)
            if value is None or not self._in_code(value) or abs(value - site.address) > HEURISTIC_REACH:
                break
            if i and self._scope is not None and (table + 4 * i) in self._scope:
                break
            targets.append(value)
        if not targets:
            return None
        return Switch(site.address, table, len(targets), targets, None, 0, "heuristic", pending,
                      self._reg(site_ins, index) if index_table is None else "")

    # -- function exploration ---------------------------------------------------------
    @staticmethod
    def _is_dynamic_tailcall(insn: Insn) -> bool:
        if insn.mem is None:
            return True  # jmp reg
        base, index, _scale, _disp = insn.mem
        return base != 0 and index == 0

    def explore(self, entry: int, source: str, entries: set[int]) -> Function:
        fn = Function(entry, source)
        self._scope = fn.insns
        try:
            self._explore(fn, entries, [entry])
            self._resolve_pending(fn, entries)
        finally:
            self._scope = None
        return fn

    # -- register constants along the control-flow graph ------------------------------------
    def _successors(self, fn: Function, va: int) -> list[int]:
        insn = self.cache[va]
        nxt = va + insn.size
        if insn.kind in (RET, STOP):
            return []
        if insn.kind == JMP:
            return [insn.target]
        if insn.kind == JMP_IND:
            return [t for sw in fn.switches if sw.site == va for t in sw.targets]
        if insn.kind == JCC:
            return [insn.target, nxt]
        return [nxt]

    def _writes(self, d, reg: str) -> str:
        """'const' (with self._last_const set), 'other' or '' for no write to reg."""
        mnemonic = d.mnemonic
        if mnemonic.startswith(("rep", "movs", "stos", "lods", "cmps", "scas")) and reg in ("esi", "edi", "ecx"):
            return "other"
        if mnemonic == "leave" and reg == "ebp":
            return "other"
        if d.id == X.X86_INS_CALL and reg in ("eax", "ecx", "edx"):
            return "other"
        if mnemonic in ("cdq", "mul", "div", "idiv") and reg in ("eax", "edx"):
            return "other"
        if mnemonic in ("imul",) and len(d.operands) == 1 and reg in ("eax", "edx"):
            return "other"
        if not d.operands:
            return ""
        if d.id == X.X86_INS_XCHG and any(o.type == X.X86_OP_REG and self._reg(d, o.reg) == reg
                                          for o in d.operands):
            return "other"
        op0 = d.operands[0]
        if op0.type != X.X86_OP_REG or self._reg(d, op0.reg) != reg:
            return ""
        if d.id in (X.X86_INS_PUSH, X.X86_INS_CMP, X.X86_INS_TEST):
            return ""
        if op0.size != 4:
            return "other"
        if d.id == X.X86_INS_MOV and d.operands[1].type == X.X86_OP_IMM:
            self._last_const = d.operands[1].imm & 0xFFFFFFFF
            return "const"
        if (d.id == X.X86_INS_XOR and d.operands[1].type == X.X86_OP_REG
                and self._reg(d, d.operands[1].reg) == reg):
            self._last_const = 0
            return "const"
        # Derived values: `mov reg, other` and `lea reg, [other + k]`.
        if d.id == X.X86_INS_MOV and d.operands[1].type == X.X86_OP_REG:
            self._last_copy = (self._reg(d, d.operands[1].reg), 0)
            return "copy"
        if d.id == X.X86_INS_LEA:
            m = d.operands[1].mem
            if m.base and not m.index:
                self._last_copy = (self._reg(d, m.base), m.disp)
                return "copy"
        return "other"

    def _reaching_constant(self, fn: Function, va: int, reg: str, depth: int = 0,
                           preds: dict | None = None) -> int | None:
        """Value of reg just before instruction va when every path from the function
        entry defines it with the same constant (directly or through copies and
        `lea reg, [other + k]`); None otherwise."""
        if depth > 4:
            return None
        if preds is None:
            preds = {}
            for a in fn.insns:
                for s in self._successors(fn, a):
                    preds.setdefault(s, []).append(a)
        value, seen, work = None, {va}, list(preds.get(va, []))
        if not work:
            return None
        while work:
            p = work.pop()
            if p in seen:
                continue
            seen.add(p)
            kind = self._writes(self._decode_detail(p), reg)
            if kind == "other":
                return None
            if kind in ("const", "copy"):
                if kind == "const":
                    k = self._last_const
                else:
                    source, offset = self._last_copy
                    base = self._reaching_constant(fn, p, source, depth + 1, preds)
                    if base is None:
                        return None
                    k = (base + offset) & 0xFFFFFFFF
                if value is not None and value != k:
                    return None
                value = k
                continue
            if p == fn.entry:
                return None  # reaches the entry undefined: value comes from the caller
            more = preds.get(p)
            if not more:
                return None
            work.extend(more)
        return value

    def _guarding_cmp(self, fn: Function, site: int, reg: str) -> int | None:
        """Entry count from `cmp reg, imm` + conditional jump found on every CFG path
        into site (the bound may sit in another block that jumps to the dispatch)."""
        preds: dict[int, list[int]] = {}
        for a in fn.insns:
            for s in self._successors(fn, a):
                preds.setdefault(s, []).append(a)
        counts, seen, work = set(), {site}, [(p, 0) for p in preds.get(site, [])]
        while work:
            p, steps = work.pop()
            if p in seen:
                continue
            seen.add(p)
            d = self._decode_detail(p)
            if (d.id == X.X86_INS_CMP and d.operands[0].type == X.X86_OP_REG
                    and self._reg(d, d.operands[0].reg) == reg and d.operands[1].type == X.X86_OP_IMM):
                # The guarding jump may come a few flag-neutral instructions later.
                after, cursor = None, p + d.size
                for _ in range(10):
                    after = self._decode_detail(cursor)
                    if after is None or after.mnemonic not in ("mov", "push", "pop", "lea", "nop"):
                        break
                    cursor += after.size
                if after is None or after.mnemonic not in ("ja", "jbe", "jae", "jb", "jnbe", "jna"):
                    return None
                imm = d.operands[1].imm & 0xFFFFFFFF
                counts.add(imm if after.mnemonic in ("jae", "jb") else imm + 1)
                continue
            if self._writes(d, reg) or steps > 24 or p == fn.entry:
                return None
            work.extend((q, steps + 1) for q in preds.get(p, []))
        return counts.pop() if len(counts) == 1 else None

    def _resolve_pending(self, fn: Function, entries: set[int]) -> None:
        """Bounds held in registers (`cmp idx, ebp`) or compared in another block are
        resolved on the complete body; newly found case targets are explored and the
        process repeats."""
        for _ in range(16):
            progress = False
            for sw in fn.switches:
                if sw.pending is None and sw.bound == "heuristic" and sw.index_reg:
                    count = self._guarding_cmp(fn, sw.site, sw.index_reg)
                    resolved = None if count is None else \
                        self._table_targets(sw.site, sw.table, count, None)
                    if resolved is not None:
                        sw.table, sw.entries, sw.targets, sw.index_entries = resolved
                        sw.bound = "flow:cmp"
                        self._mark_data(sw.table, 4 * sw.entries)
                        new = [t for t in sw.targets if t not in fn.insns]
                        if new:
                            self._explore(fn, entries, new)
                        progress = True
                    continue
                if sw.pending is None:
                    continue
                reg, cmp_va, inclusive = sw.pending
                k = self._reaching_constant(fn, cmp_va, reg)
                if k is None:
                    continue
                resolved = self._table_targets(sw.site, sw.table, k + 1 if inclusive else k,
                                               sw.index_table)
                if resolved is None:
                    continue
                sw.table, sw.entries, sw.targets, sw.index_entries = resolved
                sw.bound, sw.pending = f"flow:{reg}", None
                self._mark_data(sw.table, 4 * sw.entries)
                if sw.index_table is not None:
                    self._mark_data(sw.index_table, sw.index_entries)
                new = [t for t in sw.targets if t not in fn.insns]
                if new:
                    self._explore(fn, entries, new)
                progress = True
            if not progress:
                break
        for sw in fn.switches:
            if sw.pending is not None and not sw.targets:
                sw.pending = None
                fn.unresolved_jumps.append(sw.site)

    def _explore(self, fn: Function, entries: set[int], start: list[int]) -> None:
        entry = fn.entry
        stack = list(start)
        while stack:
            va = stack.pop()
            while True:
                if va in fn.insns:
                    break
                if va != entry and va in entries:
                    fn.tailcalls.add(va)  # fallthrough or jump into another function
                    break
                insn = self.decode(va)
                if insn is None:
                    fn.invalid.append(va)
                    break
                if self._hits_data(va, insn.size):
                    fn.into_data.append(va)
                    break
                fn.insns.add(va)
                kind = insn.kind
                if insn.code_refs:
                    fn.code_refs.update(insn.code_refs)
                if kind == RET:
                    fn.has_ret = True
                    break
                if kind == STOP:
                    break
                if kind == JMP:
                    target = insn.target
                    if not self._in_code(target):
                        fn.invalid.append(va)
                    elif target in entries and target != entry:
                        fn.tailcalls.add(target)
                    else:
                        stack.append(target)
                    break
                if kind == JMP_IND:
                    if insn.import_name:
                        fn.import_calls.add(insn.import_name)
                        if insn.import_name not in self.noreturn_imports:
                            fn.returns_via_tail = True
                        break
                    switch = self.resolve_switch(insn)
                    if switch is None and self._is_dynamic_tailcall(insn):
                        # `jmp reg` / `jmp [reg+disp]`: virtual or pointer tail call.
                        fn.dynamic_tailcalls.append(va)
                        fn.returns_via_tail = True
                    elif switch is None:
                        fn.unresolved_jumps.append(va)
                    else:
                        fn.switches.append(switch)
                        self._mark_data(switch.table, 4 * switch.entries)
                        if switch.index_table is not None:
                            self._mark_data(switch.index_table, switch.index_entries)
                        stack.extend(switch.targets)
                    break
                if kind == JCC:
                    if self._in_code(insn.target):
                        stack.append(insn.target)
                    va += insn.size
                    continue
                if kind == CALL:
                    target = insn.target
                    if self._in_code(target):
                        fn.calls.add(target)
                        if target in self.noreturn:
                            break
                    va += insn.size
                    continue
                if kind == CALL_IND:
                    if insn.import_name:
                        fn.import_calls.add(insn.import_name)
                        if insn.import_name in self.noreturn_imports:
                            break
                    va += insn.size
                    continue
                va += insn.size

    SEED_PREFIX = 6

    def _seed_rejection(self, va: int, cover: array, source: str) -> str | None:
        """Validate a low-confidence seed by its first instructions only.

        Whole-function validation is too strict: large handlers contain paths that
        only decode cleanly once every noreturn callee is known. Invalid paths found
        later are cut, not used to reject the function.
        """
        owner = cover[va - self.code_lo]
        if owner and owner != va:
            return "misaligned"
        if source == "orphan":
            # Unreferenced code counts as a function only where MSVC starts one: after
            # alignment padding. Chunks right after a jmp/ret are unreached case
            # bodies of a function whose dispatch is unresolved; they stay unexplained.
            if owner:
                return "interior"
            if self.image.u8(va - 1) not in PAD_BYTES:
                return "chunk"
        cursor = va
        for _ in range(self.SEED_PREFIX):
            insn = self.decode(cursor)
            if insn is None:
                return "invalid"
            if self.image.read(cursor, 2) == b"\0\0":
                return "zero_bytes"
            owner = cover[cursor - self.code_lo]
            if owner and owner != cursor:
                return "misaligned"
            if self._hits_data(cursor, insn.size):
                return "table"
            if insn.kind in (RET, STOP, JMP, JMP_IND):
                break
            cursor += insn.size
        return None

    def _commit_cover(self, fn: Function, cover: array) -> None:
        lo = self.code_lo
        for va in fn.insns:
            for b in range(va - lo, va - lo + self.cache[va].size):
                if not cover[b]:
                    cover[b] = va

    def data_pointer_seeds(self) -> list[int]:
        seeds = []
        for section in self.image.data_sections:
            data = section.data
            for offset in range(0, len(data) - 3, 4):
                value = struct.unpack_from("<I", data, offset)[0]
                if self._in_code(value):
                    seeds.append(value)
        return seeds

    def _discover_once(self, extra_seeds: list[tuple[int, str]]) -> None:
        functions: dict[int, Function] = {}
        rejected: dict[int, str] = {}
        cover = array("I", bytes(4 * (self.code_hi - self.code_lo)))
        entries: set[int] = set()
        # Higher-confidence seeds are always explored first, so a stray data value
        # cannot claim bytes that a real call target decodes at another alignment.
        queues: dict[str, deque] = {source: deque() for source in SOURCE_PRIORITY}
        queues["entry"].append(self.image.entry)
        queues["data_ptr"].extend(self.data_pointer_seeds())
        for va, source in extra_seeds:
            queues[source].append(va)
        strict_sources = {"data_ptr", "imm", "orphan"}
        while True:
            source = next((s for s in SOURCE_PRIORITY if queues[s]), None)
            if source is None:
                break
            va = queues[source].popleft()
            if va in functions or va in rejected:
                continue
            if source in strict_sources:
                reason = self._seed_rejection(va, cover, source)
                if reason:
                    rejected[va] = reason
                    continue
            fn = self.explore(va, source, entries)
            functions[va] = fn
            entries.add(va)
            self._commit_cover(fn, cover)
            queues["call"].extend(sorted(fn.calls))
            queues["imm"].extend(sorted(fn.code_refs))
        for loser in self._conflict_losers(functions):
            rejected[loser] = "conflict"
            del functions[loser]
            entries.discard(loser)
        # Re-explore with the final entry set so bodies stop at every known entry.
        self.functions = {va: self.explore(va, fn.source, entries) for va, fn in functions.items()}
        self.rejected = rejected

    def _conflict_losers(self, functions: dict[int, Function]) -> set[int]:
        """Functions that decode bytes another, higher-priority function decodes differently."""
        rank = {s: i for i, s in enumerate(SOURCE_PRIORITY)}
        owner: dict[int, tuple[int, int]] = {}  # byte -> (instruction start, function entry)
        losers: set[int] = set()
        ordered = sorted(functions.values(), key=lambda f: (rank[f.source], f.entry))
        for fn in ordered:
            claimed = []
            conflict = False
            for va in fn.insns:
                for b in range(va, va + self.cache[va].size):
                    prev = owner.get(b)
                    if prev is not None and prev[0] != va:
                        conflict = True
                        break
                    claimed.append((b, va))
                if conflict:
                    break
            if conflict and fn.source not in ("entry", "call"):
                losers.add(fn.entry)
                continue
            for b, va in claimed:
                owner.setdefault(b, (va, fn.entry))
        return losers

    def _compute_noreturn(self) -> set[int]:
        noreturn = set()
        changed = True
        while changed:
            changed = False
            for va, fn in self.functions.items():
                if va in noreturn or fn.has_ret or fn.returns_via_tail or fn.unresolved_jumps:
                    continue
                if any(t not in noreturn for t in fn.tailcalls):
                    continue
                noreturn.add(va)
                changed = True
        return noreturn

    def run(self, max_rounds: int = 6, orphan_rounds: int = 3) -> None:
        extra: list[tuple[int, str]] = []
        for _ in range(max_rounds):
            self._discover_once(extra)
            noreturn = self._compute_noreturn()
            if noreturn == self.noreturn:
                break
            self.noreturn = noreturn
        for _ in range(orphan_rounds):
            seeds = [(va, "orphan") for va in self.orphan_seeds()]
            if not seeds:
                break
            extra = extra + seeds
            self._discover_once(extra)
            self.noreturn = self._compute_noreturn()

    # -- classification -----------------------------------------------------------------
    def byte_map(self) -> dict[str, bytearray]:
        """Per executable section: 0 unknown, 1 code, 2 jump table, 3 index table, 4 padding."""
        maps = {}
        for s in self.image.exec_sections:
            maps[s.name] = bytearray(len(s.data))
        def mark(va, size, value):
            s = self.image.section_at(va)
            if s and s.executable:
                m = maps[s.name]
                lo = va - s.start
                for i in range(lo, min(lo + size, len(m))):
                    if m[i] == 0 or value == 1:
                        m[i] = value
        for fn in self.functions.values():
            for sw in fn.switches:
                mark(sw.table, 4 * sw.entries, 2)
                if sw.index_table is not None:
                    mark(sw.index_table, sw.index_entries, 3)
        for fn in self.functions.values():
            for va in fn.insns:
                mark(va, self.cache[va].size, 1)
        for s in self.image.exec_sections:
            m = maps[s.name]
            i = 0
            while i < len(m):
                if m[i]:
                    i += 1
                    continue
                j = i
                while j < len(m) and m[j] == 0:
                    j += 1
                # Leading and trailing alignment bytes of an unknown run are padding.
                lo, hi = i, j
                while lo < hi and s.data[lo] in PAD_BYTES:
                    lo += 1
                while hi > lo and s.data[hi - 1] in PAD_BYTES:
                    hi -= 1
                for k in list(range(i, lo)) + list(range(hi, j)):
                    m[k] = 4
                i = j
        return maps

    def unexplained_ranges(self, maps=None) -> list[tuple[int, int]]:
        maps = maps or self.byte_map()
        ranges = []
        for s in self.image.exec_sections:
            m = maps[s.name]
            i = 0
            while i < len(m):
                if m[i]:
                    i += 1
                    continue
                j = i
                while j < len(m) and m[j] == 0:
                    j += 1
                ranges.append((s.start + i, j - i))
                i = j
        return ranges

    def orphan_seeds(self) -> list[int]:
        seeds = []
        for start, size in self.unexplained_ranges():
            data = self.image.read(start, size)
            skip = 0
            while skip < len(data) and data[skip] in (0xCC, 0x90):
                skip += 1
            va = start + skip
            if va not in self.functions and va not in self.rejected:
                seeds.append(va)
        return seeds

    def quality(self) -> dict:
        """Signals that data was decoded as code or code at two alignments."""
        starts: dict[int, int] = {}
        for fn in self.functions.values():
            for va in fn.insns:
                starts[va] = self.cache[va].size
        misaligned = 0
        owner: dict[int, int] = {}
        for va in sorted(starts):
            for b in range(va, va + starts[va]):
                prev = owner.get(b)
                if prev is not None and prev != va:
                    misaligned += 1
                    break
                owner[b] = va
        table_code = 0
        for fn in self.functions.values():
            for sw in fn.switches:
                spans = [(sw.table, 4 * sw.entries)]
                if sw.index_table is not None:
                    spans.append((sw.index_table, sw.index_entries))
                for start, size in spans:
                    table_code += sum(1 for b in range(start, start + size) if b in owner)
        return {"misaligned_instructions": misaligned, "table_bytes_decoded_as_code": table_code}

    def summary(self) -> dict:
        maps = self.byte_map()
        counts = Counter()
        for m in maps.values():
            counts.update(m)
        total = sum(counts.values())
        padding = counts[4]
        explained = counts[1] + counts[2] + counts[3]
        switches = [sw for fn in self.functions.values() for sw in fn.switches]
        sources = Counter(fn.source for fn in self.functions.values())
        return {
            "functions": len(self.functions),
            "functions_by_source": dict(sorted(sources.items())),
            "noreturn_functions": len(self.noreturn),
            "rejected_seeds": dict(Counter(self.rejected.values())),
            "exec_bytes": total,
            "code_bytes": counts[1],
            "jump_table_bytes": counts[2],
            "index_table_bytes": counts[3],
            "padding_bytes": padding,
            "unexplained_bytes": counts[0],
            "coverage_non_padding": explained / (total - padding) if total > padding else 1.0,
            "switches": len(switches),
            "switches_by_bound": dict(Counter(sw.bound for sw in switches)),
            "unresolved_indirect_jumps": sum(len(fn.unresolved_jumps) for fn in self.functions.values()),
            "dynamic_tailcalls": sum(len(fn.dynamic_tailcalls) for fn in self.functions.values()),
            "functions_with_invalid_paths": sum(1 for fn in self.functions.values() if fn.invalid),
            "paths_cut_at_tables": sum(len(fn.into_data) for fn in self.functions.values()),
            **self.quality(),
        }

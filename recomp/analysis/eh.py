"""MSVC x86 C++ exception tables of vagante.exe (for throw/catch in the recompiled game).

Every function with C++ EH registers a handler stub `mov eax, FuncInfo; jmp __CxxFrameHandler3`.
From each FuncInfo this pass collects:
  funclets       unwind actions (destructors) and catch handlers, which only the tables point
                 to: they must be lifted;
  continuations  the addresses catch handlers return (`mov eax, label; ret`): after a catch the
                 runtime resumes the parent function there (_JumpToContinuation), so the lifter
                 gives the parent function a setjmp resume point for each of them.
"""
from __future__ import annotations

import struct
from dataclasses import dataclass, field

from .image import Image

CXX_FRAME_HANDLER3 = 0x008D607F  # __CxxFrameHandler3 (PDB)
MAGICS = (0x19930520, 0x19930521, 0x19930522)


@dataclass
class EhTables:
    stubs: set[int] = field(default_factory=set)
    func_infos: set[int] = field(default_factory=set)
    unwind_funclets: set[int] = field(default_factory=set)
    catch_funclets: set[int] = field(default_factory=set)

    @property
    def funclets(self) -> set[int]:
        return self.unwind_funclets | self.catch_funclets


def scan(image: Image) -> EhTables:
    t = EhTables()
    for sec in image.exec_sections:
        data = sec.data
        i = data.find(b"\xB8")
        while 0 <= i <= len(data) - 10:
            if data[i + 5] == 0xE9:
                rel = struct.unpack_from("<i", data, i + 6)[0]
                if (sec.start + i + 10 + rel) & 0xFFFFFFFF == CXX_FRAME_HANDLER3:
                    t.stubs.add(sec.start + i)
                    t.func_infos.add(struct.unpack_from("<I", data, i + 1)[0])
            i = data.find(b"\xB8", i + 1)
    for fi in sorted(t.func_infos):
        if image.u32(fi) not in MAGICS:
            continue
        max_state, unwind_map, n_try, try_map = (image.u32(fi + k) for k in (4, 8, 12, 16))
        for s in range(min(max_state or 0, 4096)):
            action = image.u32(unwind_map + 8 * s + 4)
            if action and image.is_exec(action):
                t.unwind_funclets.add(action)
        for b in range(min(n_try or 0, 1024)):
            entry = try_map + 20 * b
            n_catches, handlers = image.u32(entry + 12), image.u32(entry + 16)
            for h in range(min(n_catches or 0, 256)):
                address = image.u32(handlers + 16 * h + 12)
                if address and image.is_exec(address):
                    t.catch_funclets.add(address)
    return t


def continuations(discovery, catch_funclets: set[int], entries: set[int]) -> set[int]:
    """Labels the catch funclets return: `mov eax, imm32` directly followed by `ret`."""
    found: set[int] = set()
    for f in catch_funclets:
        fn = discovery.explore(f, "call", entries)
        for va in fn.insns:
            ins = discovery._decode_detail(va)
            if ins.mnemonic != "mov" or ins.op_str.split(",")[0].strip() != "eax":
                continue
            nxt = va + ins.size
            if nxt not in fn.insns or discovery._decode_detail(nxt).mnemonic != "ret":
                continue
            op = ins.operands[1]
            if op.type == 2:  # X86_OP_IMM
                target = op.imm & 0xFFFFFFFF
                if discovery.image.is_exec(target):
                    found.add(target)
    return found

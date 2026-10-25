"""Differential verification of lifted functions against the original x86 in Unicorn.

A case is an initial state (entry, registers, x87 control word, a data block that
register pointers refer to, stack arguments). The same cases run through the
original bytes in Unicorn and through recomp/tests/diff_runner.c linked with the
lifted C; final registers, x87 stack top and the data block must match.
"""
from __future__ import annotations

import os
import random
import shutil
import struct
import subprocess
import sys
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / "local/python-deps"))

from unicorn import (UC_ARCH_X86, UC_HOOK_CODE, UC_HOOK_MEM_INVALID, UC_MODE_32,  # noqa: E402
                     Uc, UcError)
from unicorn import x86_const as U  # noqa: E402

DATA, DATA_SIZE, ARGS_SIZE = 0x1000000, 1024, 64
STACK, STACK_SIZE, STOP = 0x2000000, 0x10000, 0x3000000
ST_IN, STUBS, STUBS_SIZE = STOP + 0x800, 0x4000000, 0x10000
REGS = (U.UC_X86_REG_EAX, U.UC_X86_REG_ECX, U.UC_X86_REG_EDX, U.UC_X86_REG_EBX,
        U.UC_X86_REG_ESP, U.UC_X86_REG_EBP, U.UC_X86_REG_ESI, U.UC_X86_REG_EDI)
RESULT_SIZE = 1 + 32 + 2 + 1 + 10 + DATA_SIZE
GCC = Path(r"C:\msys64\ucrt64\bin\gcc.exe")


@dataclass
class Case:
    entry: int
    regs: tuple
    cw: int
    data: bytes
    args: bytes
    st_in: float = 0.0  # pushed on the x87 stack before the call (x87 arguments)

    def pack(self) -> bytes:
        return (struct.pack("<I8IH", self.entry, *self.regs, self.cw) + self.data + self.args
                + struct.pack("<d", self.st_in))


@dataclass
class Result:
    status: int
    regs: tuple
    sw: int
    depth: int
    st0: bytes
    data: bytes

    @classmethod
    def unpack(cls, raw: bytes) -> "Result":
        status = raw[0]
        regs = struct.unpack_from("<8I", raw, 1)
        sw, depth = struct.unpack_from("<HB", raw, 33)
        return cls(status, regs, sw, depth, raw[36:46], raw[46:46 + DATA_SIZE])


def random_case(entry: int, rng: random.Random, cw: int) -> Case:
    """Pointers into the data block in every register; floats, small ints and
    pointers in the block and on the stack. Ints stay non-negative: a negative
    small int read as a float is a NaN with payload, and NaN payload propagation
    is outside the x87 model (see docs/F1_LIFTER.md)."""
    def word() -> bytes:
        kind = rng.randrange(4)
        if kind == 0:
            return struct.pack("<f", rng.uniform(-1000, 1000))
        if kind == 1:
            return struct.pack("<i", rng.randint(0, 50))
        if kind == 2:
            return struct.pack("<I", DATA + 4 * rng.randrange(DATA_SIZE // 8))
        return struct.pack("<f", rng.choice([0.0, 1.0, -1.0, 0.5, 2.0, 640.0, 480.0]))
    data = b"".join(word() for _ in range(DATA_SIZE // 4))
    args = b"".join(word() for _ in range(ARGS_SIZE // 4))
    regs = tuple(DATA + 0x40 * i for i in range(8))
    st_in = rng.choice([rng.uniform(-1e6, 1e6), float(rng.randint(-1000, 1000)), 0.5, -2.5, 3.0])
    return Case(entry, regs, cw, data, args, st_in)


class UnicornReference:
    def __init__(self, image_bytes: bytes, base: int):
        self.image = (base, image_bytes)
        self.fault = False
        self.x87_executed = False
        self._create()

    def _create(self) -> None:
        # A run that ends in UcError can leave the engine unusable for later runs,
        # so the engine is rebuilt after every failed case.
        base, image_bytes = self.image
        self.uc = Uc(UC_ARCH_X86, UC_MODE_32)
        self.uc.mem_map(base, (len(image_bytes) + 0xFFF) & ~0xFFF)
        self.uc.mem_write(base, image_bytes)
        self.uc.mem_map(DATA, 0x1000)
        self.uc.mem_map(STACK, STACK_SIZE)
        self.uc.mem_map(STOP, 0x1000)
        self.uc.mem_map(STUBS, STUBS_SIZE)
        self.stubs: dict[int, int] = {}
        self.uc.hook_add(UC_HOOK_MEM_INVALID, self._invalid)
        self.uc.hook_add(UC_HOOK_CODE, self._code, begin=base, end=base + len(image_bytes))

    def _stub(self, entry: int) -> int:
        if entry not in self.stubs:
            stub = STUBS + 16 * len(self.stubs)
            if stub + 16 > STUBS + STUBS_SIZE:
                raise RuntimeError("stub area exhausted")
            self.stubs[entry] = stub
        stub = self.stubs[entry]
        self.uc.mem_write(stub, b"\xdd\x05" + struct.pack("<I", ST_IN) + b"\xe9"
                          + struct.pack("<i", entry - (stub + 11)))
        return stub

    def _invalid(self, uc, access, address, size, value, user):
        self.fault = True
        return False

    def _code(self, uc, address, size, user):
        b = uc.mem_read(address, 2)
        k = 1 if b[0] in (0x66, 0x9B) else 0
        if 0xD8 <= b[k] <= 0xDF:
            self.x87_executed = True

    def run(self, case: Case, max_instructions: int = 20000) -> Result | None:
        """Returns None when the original code faults or does not return to STOP."""
        uc = self.uc
        base, image = self.image
        uc.mem_write(base, image)  # undo writes to globals from earlier cases
        uc.mem_write(DATA, case.data)
        esp = STACK + STACK_SIZE - ARGS_SIZE - 4
        uc.mem_write(esp, struct.pack("<I", STOP) + case.args)
        for reg, value in zip(REGS, case.regs):
            uc.reg_write(reg, value)
        uc.reg_write(U.UC_X86_REG_ESP, esp)
        uc.reg_write(U.UC_X86_REG_EFLAGS, 0x2)
        uc.reg_write(U.UC_X86_REG_FPCW, case.cw)
        uc.reg_write(U.UC_X86_REG_FPSW, 0)
        uc.reg_write(U.UC_X86_REG_FPTAG, 0xFFFF)
        for i in range(8):  # no stale register contents from earlier cases
            uc.reg_write(U.UC_X86_REG_FP0 + i, (0, 0))
        # Stub `fld qword [ST_IN]; jmp entry` (stack untouched). Each entry gets its
        # own stub address and only ST_IN (data) changes between runs: Unicorn keeps
        # translated blocks, so rewriting code at one address would run stale code.
        stub = self._stub(case.entry)
        uc.mem_write(ST_IN, struct.pack("<d", case.st_in))
        self.fault = False
        try:
            uc.emu_start(stub, STOP, count=max_instructions)
        except UcError:
            self._create()
            return None
        if self.fault or uc.reg_read(U.UC_X86_REG_EIP) != STOP:
            self._create()
            return None
        sw = uc.reg_read(U.UC_X86_REG_FPSW)
        top = (sw >> 11) & 7
        depth = (8 - top) & 7  # cases start with TOP=0 and push st_in (depth 1)
        st0 = b"\0" * 10
        if depth:
            mantissa, exponent = uc.reg_read(U.UC_X86_REG_FP0 + top)
            st0 = struct.pack("<QH", mantissa, exponent)
        regs = tuple(uc.reg_read(r) for r in REGS)
        return Result(0, regs, sw, depth, st0, bytes(uc.mem_read(DATA, DATA_SIZE)))


def build_runner(lifted_c: Path, output: Path) -> Path:
    runtime = ROOT / "recomp/runtime"
    cmd = [str(GCC), "-std=gnu11", "-O1", "-frounding-math", f"-I{runtime}",
           str(runtime / "guest.c"), str(lifted_c), str(ROOT / "recomp/tests/diff_runner.c"),
           "-lm", "-o", str(output)]
    env = dict(os.environ, PATH=str(GCC.parent) + os.pathsep + os.environ.get("PATH", ""))
    subprocess.run(cmd, check=True, env=env, capture_output=True, text=True)
    return output


def run_lifted(runner: Path, exe: Path, cases: list[Case], workdir: Path) -> list[Result]:
    cases_bin, results_bin = workdir / "cases.bin", workdir / "results.bin"
    cases_bin.write_bytes(b"".join(c.pack() for c in cases))
    subprocess.run([str(runner), str(exe), str(cases_bin), str(results_bin)], check=True,
                   capture_output=True, timeout=600)
    raw = results_bin.read_bytes()
    return [Result.unpack(raw[i:i + RESULT_SIZE]) for i in range(0, len(raw), RESULT_SIZE)]


def gcc_available() -> bool:
    return GCC.exists() or shutil.which("gcc") is not None


def x87_stack_fault(result: Result) -> bool:
    """The original popped an empty x87 register: it takes more x87 arguments than
    the single st_in value a case provides."""
    return bool(result.sw & 0x40)


def compare(reference: Result, lifted: Result) -> list[str]:
    """Differences that matter for the calling convention: return registers,
    callee-saved registers, esp, x87 stack depth/top value and memory."""
    diffs = []
    if lifted.status:
        return ["lifted code raised guest_fault"]
    names = ("eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi")
    for i in (0, 2, 3, 4, 5, 6, 7):
        if reference.regs[i] != lifted.regs[i]:
            diffs.append(f"{names[i]} {reference.regs[i]:08x} != {lifted.regs[i]:08x}")
    if reference.depth != lifted.depth:
        diffs.append(f"x87 depth {reference.depth} != {lifted.depth}")
    elif reference.depth and reference.st0 != lifted.st0:
        diffs.append(f"st0 {reference.st0.hex()} != {lifted.st0.hex()}")
    if reference.data != lifted.data:
        first = next(i for i in range(DATA_SIZE) if reference.data[i] != lifted.data[i]) & ~3
        diffs.append(f"data at {first:#x}: {reference.data[first:first + 4].hex()} != "
                     f"{lifted.data[first:first + 4].hex()}")
    return diffs

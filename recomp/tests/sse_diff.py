#!/usr/bin/env python3
"""Differential test of the SSE lifting against real x86 (Unicorn).

Every instruction under test is wrapped in a tiny function of a synthetic image:

    movups xmm0,[esi]; movups xmm1,[esi+16]; mov eax,[esi+32]
    <instruction>                      ; xmm0/eax destination, xmm1/eax/[esi+16] source
    movups [edi],xmm0; movups [edi+16],xmm1; mov [edi+32],eax
    pushfd; pop ebx; mov [edi+36],ebx; ret

The same inputs run through the original bytes in Unicorn and through the lifted C;
xmm0, xmm1, eax and the arithmetic flags must match bit for bit.

    python recomp/tests/sse_diff.py [cases_per_instruction]
"""
from __future__ import annotations

import os
import random
import struct
import subprocess
import sys
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT))
sys.path.insert(0, str(ROOT / "local/python-deps"))

from unicorn import UC_ARCH_X86, UC_MODE_32, Uc  # noqa: E402
from unicorn import x86_const as U  # noqa: E402

from recomp.analysis.discover import Discovery  # noqa: E402
from recomp.analysis.image import Image, Section  # noqa: E402
from recomp.lift.lifter import Lifter  # noqa: E402

GCC = Path(r"C:\msys64\ucrt64\bin\gcc.exe")
BASE, CODE = 0x400000, 0x401000
DATA_IN, DATA_OUT, STACK_TOP = 0x100000, 0x100100, 0x10F000
FLAGS_MASK = 0x8C5  # CF PF ZF SF OF (AF is not modelled)

PRE = bytes.fromhex("0f1006 0f104e10 8b4620".replace(" ", ""))
POST = bytes.fromhex("0f1107 0f114f10 894720 9c 5b 895f24 c3".replace(" ", ""))


def ins(hexbytes: str) -> bytes:
    return bytes.fromhex(hexbytes.replace(" ", ""))


# name -> encoding. Register forms use xmm0 as destination and xmm1 as source (modrm C1);
# "m" variants read the source from [esi+16] (modrm 46 10).
TESTS: dict[str, bytes] = {}
for kind, prefix in (("ss", "f3"), ("sd", "f2"), ("ps", ""), ("pd", "66")):
    for name, op in (("add", "58"), ("mul", "59"), ("sub", "5c"), ("min", "5d"), ("div", "5e"),
                     ("max", "5f"), ("sqrt", "51")):
        TESTS[name + kind] = ins(f"{prefix}0f{op}c1")
        TESTS[name + kind + " m"] = ins(f"{prefix}0f{op}4610")
    for pred in range(8):
        TESTS[f"cmp{kind} {pred}"] = ins(f"{prefix}0fc2c1{pred:02x}")
    TESTS["mov" + ("ss" if kind == "ss" else "sd" if kind == "sd" else "aps" if kind == "ps" else "apd")] = \
        ins(f"{prefix}0f{'10' if kind in ('ss', 'sd') else '28'}c1")
TESTS["movss load"] = ins("f30f104610")
TESTS["movsd load"] = ins("f20f104610")
for kind, prefix in (("ps", ""), ("pd", "66")):
    for name, op in (("and", "54"), ("andn", "55"), ("or", "56"), ("xor", "57")):
        TESTS[name + kind] = ins(f"{prefix}0f{op}c1")
TESTS.update({
    "comiss": ins("0f2fc1"), "ucomiss": ins("0f2ec1"), "comisd": ins("660f2fc1"),
    "ucomisd": ins("660f2ec1"), "comiss m": ins("0f2f4610"),
    "cvtss2sd": ins("f30f5ac1"), "cvtsd2ss": ins("f20f5ac1"), "cvtps2pd": ins("0f5ac1"),
    "cvtpd2ps": ins("660f5ac1"), "cvtdq2ps": ins("0f5bc1"), "cvttps2dq": ins("f30f5bc1"),
    "cvtps2dq": ins("660f5bc1"), "cvtdq2pd": ins("f30fe6c1"),
    "cvttss2si": ins("f30f2cc1"), "cvtss2si": ins("f30f2dc1"), "cvttsd2si": ins("f20f2cc1"),
    "cvtsd2si": ins("f20f2dc1"), "cvtsi2ss": ins("f30f2ac0"), "cvtsi2sd": ins("f20f2ac0"),
    "cvttss2si m": ins("f30f2c4610"),
    "shufps 1b": ins("0fc6c11b"), "shufps e4": ins("0fc6c1e4"), "shufpd 1": ins("660fc6c101"),
    "pshufd 4e": ins("660f70c14e"), "pshuflw 1b": ins("f20f70c11b"), "pshufhw 4e": ins("f30f70c14e"), "unpcklps": ins("0f14c1"), "unpckhps": ins("0f15c1"),
    "unpcklpd": ins("660f14c1"), "movlhps": ins("0f16c1"), "movhlps": ins("0f12c1"),
    "paddd": ins("660ffec1"), "psubd": ins("660ffac1"), "paddw": ins("660ffdc1"),
    "psubw": ins("660ff9c1"), "paddb": ins("660ffcc1"), "paddq": ins("660fd4c1"),
    "pxor": ins("660fefc1"), "por": ins("660febc1"), "pand": ins("660fdbc1"), "pandn": ins("660fdfc1"),
    "punpckldq": ins("660f62c1"), "punpckhdq": ins("660f6ac1"), "punpcklbw": ins("660f60c1"),
    "punpcklwd": ins("660f61c1"), "punpcklqdq": ins("660f6cc1"), "punpckhbw": ins("660f68c1"),
    "pmaddwd": ins("660ff5c1"), "pmulhw": ins("660fe5c1"), "pmullw": ins("660fd5c1"),
    "packssdw": ins("660f6bc1"), "packuswb": ins("660f67c1"),
    "pcmpeqd": ins("660f76c1"), "pcmpgtd": ins("660f66c1"), "pcmpeqb": ins("660f74c1"),
    "psrad 5": ins("660f72e005"), "psrld 5": ins("660f72d005"), "pslld 5": ins("660f72f005"),
    "psrlq 7": ins("660f73d007"), "psllq 7": ins("660f73f007"), "psrldq 3": ins("660f73d803"),
    "pslldq 3": ins("660f73f803"), "psraw 3": ins("660f71e003"), "psrad xmm": ins("660fe2c1"),
    "psllq xmm": ins("660ff3c1"), "psrlq 70": ins("660f73d046"),
    "movq": ins("f30f7ec1"), "movd xmm,eax": ins("660f6ec0"), "movd eax,xmm": ins("660f7ec8"),
    "movmskps": ins("0f50c1"), "pmovmskb": ins("660fd7c1"), "pextrw 5": ins("660fc5c105"),
    "pinsrw 2": ins("660fc4c002"), "movups m": ins("0f104610"), "movlps m": ins("0f124610"),
    "movhps m": ins("0f164610"),
})


def special_float(rng: random.Random) -> bytes:
    v = rng.choice([0.0, -0.0, 1.0, -1.0, 0.5, 1.5, 2.5, -2.5, 1e30, -1e30, 3e9, -3e9,
                    2147483647.0, -2147483648.0, float("inf"), float("-inf"), float("nan"),
                    rng.uniform(-1000, 1000), rng.uniform(-1, 1), rng.uniform(-1e6, 1e6)])
    return struct.pack("<f", v)


def special_double(rng: random.Random) -> bytes:
    v = rng.choice([0.0, -0.0, 1.0, -1.0, 0.5, 2.5, -2.5, 1e300, 3e9, -3e9, 2147483647.5,
                    float("inf"), float("-inf"), float("nan"), rng.uniform(-1000, 1000)])
    return struct.pack("<d", v)


def random_input(rng: random.Random) -> bytes:
    regs = b""
    for _ in range(2):  # xmm0, xmm1
        kind = rng.randrange(3)
        if kind == 0:
            regs += b"".join(special_float(rng) for _ in range(4))
        elif kind == 1:
            regs += b"".join(special_double(rng) for _ in range(2))
        else:
            regs += bytes(rng.randrange(256) for _ in range(16))
    eax = rng.choice([0, 1, -1, 7, -12345, 2 ** 31 - 1, -2 ** 31, rng.randint(-2 ** 31, 2 ** 31 - 1)])
    return regs + struct.pack("<i", eax) + bytes(4)


def build_image(names: list[str]) -> tuple[Image, dict[str, int]]:
    code = bytearray()
    entries = {}
    for name in names:
        entries[name] = CODE + len(code)
        code += PRE + TESTS[name] + POST
        code += b"\xcc" * (-len(code) % 16)
    return Image(BASE, CODE, [Section(".text", CODE, bytes(code), True)]), entries


def run_unicorn(code_image: Image, entry: int, data: bytes) -> bytes:
    uc = Uc(UC_ARCH_X86, UC_MODE_32)
    uc.mem_map(CODE, 0x100000)
    uc.mem_write(CODE, code_image.sections[0].data)
    uc.mem_map(DATA_IN, 0x10000)
    uc.mem_write(DATA_IN, data)
    stop = CODE + 0xFFFF0
    esp = STACK_TOP - 4
    uc.mem_write(esp, struct.pack("<I", stop))
    uc.reg_write(U.UC_X86_REG_ESP, esp)
    uc.reg_write(U.UC_X86_REG_ESI, DATA_IN)
    uc.reg_write(U.UC_X86_REG_EDI, DATA_OUT)
    uc.reg_write(U.UC_X86_REG_EFLAGS, 0x2)
    uc.emu_start(entry, stop)
    out = bytearray(uc.mem_read(DATA_OUT, 40))
    flags = struct.unpack_from("<I", out, 36)[0] & FLAGS_MASK
    struct.pack_into("<I", out, 36, flags)
    return bytes(out)


HARNESS = r"""
#include <stdio.h>
#include <stdlib.h>
#include "guest.h"
#ifdef _WIN32
#include <fcntl.h>
#include <io.h>
#endif
%(protos)s
static GuestFn fns[] = { %(fns)s };
int main(void) {
#ifdef _WIN32
    _setmode(_fileno(stdin), _O_BINARY); _setmode(_fileno(stdout), _O_BINARY);
#endif
    if (guest_mem_init() || guest_map(0x100000, 0x10000)) { fprintf(stderr, "mem\n"); return 2; }
    unsigned idx; unsigned char in[40];
    while (fread(&idx, 4, 1, stdin) == 1 && fread(in, 40, 1, stdin) == 1) {
        Cpu c; memset(&c, 0, sizeof c);
        memcpy(G(0x100000), in, 40);
        c.esi.d = 0x100000; c.edi.d = 0x100100; c.esp.d = 0x10F000; push32(&c, 0xDEAD0000u);
        fns[idx](&c);
        unsigned char out[40]; memcpy(out, G(0x100100), 40);
        unsigned f; memcpy(&f, out + 36, 4); f &= 0x%(mask)Xu; memcpy(out + 36, &f, 4);
        fwrite(out, 40, 1, stdout);
    }
    return 0;
}
"""


NAN_ARITH = {f"{op}{k}{m}": k for op in ("add", "sub", "mul", "div")
             for k in ("ss", "ps", "sd", "pd") for m in ("", " m")}


def qemu_nan_pair(name: str, data: bytes) -> bool:
    """Oracle exception: when both operands hold a NaN in the same lane, Unicorn (QEMU)
    sometimes returns the second operand's NaN; the Intel SDM and real hardware return
    the first. The lifted code follows the SDM, so those cases are not compared."""
    kind = NAN_ARITH.get(name)
    if not kind:
        return False
    fmt, lanes = ("<4f", 4) if kind in ("ss", "ps") else ("<2d", 2)
    if kind in ("ss", "sd"):
        lanes = 1
    a = struct.unpack_from(fmt, data, 0)[:lanes]
    b = struct.unpack_from(fmt, data, 16)[:lanes]
    return any(x != x and y != y for x, y in zip(a, b))

def main() -> int:
    cases_per = int(sys.argv[1]) if len(sys.argv) > 1 else 400
    names = list(TESTS)
    image, entries = build_image(names)
    lifter = Lifter(Discovery(image), set(entries.values()))
    result = lifter.lift(sorted(entries.values()))
    if result.faults:
        for va, text in sorted(result.faults.items()):
            name = next(n for n, e in entries.items() if e <= va < e + 64)
            print(f"FAULT {name}: {text}")
    rng = random.Random(1234)
    order = sorted(entries.values())
    with tempfile.TemporaryDirectory() as tmp:
        tmp = Path(tmp)
        (tmp / "lifted.c").write_text(result.source, encoding="utf-8")
        protos = "\n".join(f"void f_{e:08x}(Cpu *c);" for e in order)
        (tmp / "main.c").write_text(HARNESS % {"protos": protos, "mask": FLAGS_MASK,
                                               "fns": ", ".join(f"f_{e:08x}" for e in order)})
        exe = tmp / "sse_diff.exe"
        env = dict(os.environ, PATH=str(GCC.parent) + os.pathsep + os.environ["PATH"])
        build = subprocess.run([str(GCC), "-std=gnu11", "-O1", "-frounding-math", "-w",
                                f"-I{ROOT / 'recomp/runtime'}", str(tmp / "lifted.c"), str(tmp / "main.c"),
                                str(ROOT / "recomp/runtime/guest.c"), "-lm", "-o", str(exe)],
                               capture_output=True, text=True, env=env)
        if build.returncode:
            print(build.stderr[-3000:])
            return 1
        inputs, expected, labels = [], [], []
        for name in names:
            for _ in range(cases_per):
                data = random_input(rng)
                inputs.append(struct.pack("<I", order.index(entries[name])) + data)
                expected.append(run_unicorn(image, entries[name], data))
                labels.append((name, data))
        run = subprocess.run([str(exe)], input=b"".join(inputs), capture_output=True, env=env)
        got = run.stdout
        if len(got) < 40 * len(labels):
            print(f"harness exit {run.returncode}: {run.stderr.decode(errors='replace')[-2000:]} ({len(got)} bytes)")
    failures: dict[str, list] = {}
    for k, (label, want) in enumerate(zip(labels, expected)):
        have = got[40 * k:40 * k + 40]
        if have != want and not qemu_nan_pair(label[0], label[1]):
            failures.setdefault(label[0], []).append((label[1], want, have))
    for name in names:
        bad = failures.get(name, [])
        if bad:
            data, want, have = bad[0]
            print(f"MISMATCH {name}: {len(bad)}/{cases_per}\n  in   {data.hex()}\n  want {want.hex()}\n  have {have.hex()}")
    print(f"{len(names)} instructions, {len(names) * cases_per} cases, "
          f"{sum(len(v) for v in failures.values())} mismatches, {len(result.faults)} faults")
    return 1 if failures or result.faults else 0


if __name__ == "__main__":
    sys.exit(main())

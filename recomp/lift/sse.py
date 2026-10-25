"""SSE/SSE2 (and the few MMX-style integer ops on xmm) for the lifter.

Vagante is compiled with /arch:SSE2, so its floating point is scalar SSE instead of x87.
Every instruction here works on the explicit `c->xmm[]` state of the runtime; memory
operands are loaded into a temporary register value first (`xld`), so each handler only
deals with register values. Results follow x86 semantics where C would differ: NaN and
zero handling of min/max, out-of-range float->int conversions, unordered compares.
"""
from __future__ import annotations

from capstone import x86_const as X


class SseUnsupported(Exception):
    pass


SCALAR_OPS = {"add": "+", "sub": "-", "mul": "*", "div": "/"}
CMP_SUFFIX = {"eq": 0, "lt": 1, "le": 2, "unord": 3, "neq": 4, "nlt": 5, "nle": 6, "ord": 7}
LOGIC_OPS = {"and": "&", "or": "|", "xor": "^"}
INT_LANES = {"b": (16, "b", "int8_t"), "w": (8, "w", "int16_t"), "d": (4, "d", "int32_t"),
             "q": (2, "q", "int64_t")}


def _xreg(ins, op) -> int | None:
    """xmm register number, None for memory, SseUnsupported for any other register
    (MMX mm0-7 is not modelled)."""
    if op.type == X.X86_OP_REG:
        name = ins.reg_name(op.reg)
        if name.startswith("xmm"):
            return int(name[3:])
        if name.startswith("mm"):
            raise SseUnsupported("mmx register")
    return None


def _xreq(ins, op) -> int:
    n = _xreg(ins, op)
    if n is None:
        raise SseUnsupported("operand is not xmm")
    return n


class Sse:
    def __init__(self, lifter):
        self.L = lifter

    # -- operands ---------------------------------------------------------------------------
    def load(self, ins, op, size: int, var: str = "s") -> str:
        """C statement declaring `Xmm var` with the operand's value (size bytes from memory)."""
        n = _xreg(ins, op)
        if n is not None:
            return f"Xmm {var} = c->xmm[{n}];"
        if op.type == X.X86_OP_MEM:
            return f"Xmm {var} = xld({self.L.address(ins, op)}, {size});"
        raise SseUnsupported("operand")

    def dst(self, ins, op) -> str:
        n = _xreg(ins, op)
        if n is None:
            raise SseUnsupported("destination is not xmm")
        return f"c->xmm[{n}]"

    def block(self, *stmts: str) -> str:
        return "{ " + " ".join(stmts) + " }"

    # -- dispatch ---------------------------------------------------------------------------
    def lift(self, ins) -> str:
        m = ins.mnemonic.split()[-1]
        ops = ins.operands
        handler = getattr(self, "op_" + m, None)
        if handler:
            return handler(ins, ops)
        # scalar/packed arithmetic: addss addsd addps addpd ...
        for name, sym in SCALAR_OPS.items():
            for kind in ("ss", "sd", "ps", "pd"):
                if m == name + kind:
                    macro = "SSE_ARITH_F" if kind in ("ss", "ps") else "SSE_ARITH_D"
                    return self.arith(ins, ops, kind,
                                      lambda a, b, s=sym, mc=macro: f"{mc}({a}, {s}, {b})")
        for kind in ("ss", "sd", "ps", "pd"):
            if m in ("min" + kind, "max" + kind):
                fn = ("sse_" + m[:3]) + ("f" if kind in ("ss", "ps") else "d")
                return self.arith(ins, ops, kind, lambda a, b, fn=fn: f"{fn}({a}, {b})")
            if m == "sqrt" + kind:
                fn = "__builtin_sqrtf" if kind in ("ss", "ps") else "__builtin_sqrt"
                return self.arith(ins, ops, kind, lambda a, b, fn=fn: f"{fn}({b})")
        for kind in ("ps", "pd"):
            for name, sym in LOGIC_OPS.items():
                if m == name + kind:
                    return self.bitwise(ins, ops, lambda a, b, s=sym: f"{a} {s} {b}")
            if m == "andn" + kind:
                return self.bitwise(ins, ops, lambda a, b: f"~{a} & {b}")
        if m in ("pand", "por", "pxor"):
            return self.bitwise(ins, ops, lambda a, b, s=LOGIC_OPS[m[1:]]: f"{a} {s} {b}")
        if m == "pandn":
            return self.bitwise(ins, ops, lambda a, b: f"~{a} & {b}")
        if m.startswith("cmp") and m[-2:] in ("ss", "sd", "ps", "pd") and len(ops) >= 2:
            return self.compare(ins, ops, m)
        if m[:4] in ("padd", "psub") and m[4:] in INT_LANES:
            return self.int_lanes(ins, ops, m[4:], "+" if m[1:4] == "add" else "-")
        if m[:6] == "pcmpeq" and m[6:] in INT_LANES:
            return self.int_cmp(ins, ops, m[6:], "==")
        if m[:6] == "pcmpgt" and m[6:] in INT_LANES:
            return self.int_cmp(ins, ops, m[6:], ">")
        if m in ("psllw", "pslld", "psllq", "psrlw", "psrld", "psrlq", "psraw", "psrad"):
            return self.shift(ins, ops, m)
        raise SseUnsupported(m)

    # -- moves --------------------------------------------------------------------------------
    def move128(self, ins, ops) -> str:
        d, s = ops
        if _xreg(ins, d) is not None:
            return self.block(self.load(ins, s, 16), f"{self.dst(ins, d)} = s;")
        n = _xreq(ins, s)
        return f"xst({self.L.address(ins, d)}, &c->xmm[{n}], 16);"

    op_movaps = op_movups = op_movapd = op_movupd = op_movdqa = op_movdqu = move128
    op_lddqu = move128

    def move_scalar(self, ins, ops, size: int) -> str:
        d, s = ops
        dn, sn = _xreg(ins, d), _xreg(ins, s)
        if dn is not None and sn is not None:  # register to register: low lane only
            lanes = "d[0]" if size == 4 else "q[0]"
            return f"c->xmm[{dn}].{lanes} = c->xmm[{sn}].{lanes};"
        if dn is not None:  # from memory: upper lanes cleared
            return f"c->xmm[{dn}] = xld({self.L.address(ins, s)}, {size});"
        return f"xst({self.L.address(ins, d)}, &c->xmm[{_xreq(ins, s)}], {size});"

    def op_movss(self, ins, ops):
        return self.move_scalar(ins, ops, 4)

    def op_movsd(self, ins, ops):
        if not any(_xreg(ins, op) is not None for op in ops):  # string movsd: main lifter
            raise SseUnsupported("movsd string")
        return self.move_scalar(ins, ops, 8)

    def op_movd(self, ins, ops):
        d, s = ops
        dn = _xreg(ins, d)
        if dn is not None:
            return self.block(f"uint32_t v = {self.L.read(ins, s, 4)};",
                              f"memset(&c->xmm[{dn}], 0, 16); c->xmm[{dn}].d[0] = v;")
        sn = _xreg(ins, s)
        if sn is None:
            raise SseUnsupported("movd mmx")
        return self.L.write(ins, d, f"c->xmm[{sn}].d[0]")

    def op_movq(self, ins, ops):
        d, s = ops
        dn, sn = _xreg(ins, d), _xreg(ins, s)
        if dn is not None and sn is not None:
            return self.block(f"uint64_t v = c->xmm[{sn}].q[0];",
                              f"memset(&c->xmm[{dn}], 0, 16); c->xmm[{dn}].q[0] = v;")
        if dn is not None and s.type == X.X86_OP_MEM:
            return f"c->xmm[{dn}] = xld({self.L.address(ins, s)}, 8);"
        if sn is not None and d.type == X.X86_OP_MEM:
            return f"xst({self.L.address(ins, d)}, &c->xmm[{sn}], 8);"
        raise SseUnsupported("movq mmx")

    def half(self, ins, ops, lane: int) -> str:
        """movlps/movhps/movlpd/movhpd: one 64-bit half to or from memory."""
        d, s = ops
        dn = _xreg(ins, d)
        if dn is not None:
            return f"memcpy(&c->xmm[{dn}].q[{lane}], G({self.L.address(ins, s)}), 8);"
        sn = _xreq(ins, s)
        return f"memcpy(G({self.L.address(ins, d)}), &c->xmm[{sn}].q[{lane}], 8);"

    def op_movlps(self, ins, ops): return self.half(ins, ops, 0)
    op_movlpd = op_movlps

    def op_movhps(self, ins, ops): return self.half(ins, ops, 1)
    op_movhpd = op_movhps

    def op_movlhps(self, ins, ops):
        return f"c->xmm[{_xreq(ins, ops[0])}].q[1] = c->xmm[{_xreq(ins, ops[1])}].q[0];"

    def op_movhlps(self, ins, ops):
        return f"c->xmm[{_xreq(ins, ops[0])}].q[0] = c->xmm[{_xreq(ins, ops[1])}].q[1];"

    def op_movmskps(self, ins, ops):
        n = _xreq(ins, ops[1])
        return self.L.write(ins, ops[0], f"(uint32_t)((c->xmm[{n}].d[0] >> 31) | (c->xmm[{n}].d[1] >> 31) << 1"
                                         f" | (c->xmm[{n}].d[2] >> 31) << 2 | (c->xmm[{n}].d[3] >> 31) << 3)")

    def op_movmskpd(self, ins, ops):
        n = _xreq(ins, ops[1])
        return self.L.write(ins, ops[0], f"(uint32_t)((c->xmm[{n}].q[0] >> 63) | (c->xmm[{n}].q[1] >> 63) << 1)")

    def op_pmovmskb(self, ins, ops):
        n = _xreq(ins, ops[1])
        return self.block("uint32_t r = 0;",
                          f"for (int k = 0; k < 16; ++k) r |= (uint32_t)(c->xmm[{n}].b[k] >> 7) << k;",
                          self.L.write(ins, ops[0], "r"))

    # -- arithmetic -----------------------------------------------------------------------
    def arith(self, ins, ops, kind: str, expr) -> str:
        d = self.dst(ins, ops[0])
        size = {"ss": 4, "sd": 8, "ps": 16, "pd": 16}[kind]
        lane, count = {"ss": ("f", 1), "sd": ("g", 1), "ps": ("f", 4), "pd": ("g", 2)}[kind]
        body = " ".join(f"{d}.{lane}[{k}] = {expr(f'{d}.{lane}[{k}]', f's.{lane}[{k}]')};"
                        for k in range(count))
        return self.block(self.load(ins, ops[1], size), body)

    def bitwise(self, ins, ops, expr) -> str:
        d = self.dst(ins, ops[0])
        body = " ".join(f"{d}.q[{k}] = {expr(f'{d}.q[{k}]', f's.q[{k}]')};" for k in range(2))
        return self.block(self.load(ins, ops[1], 16), body)

    def compare(self, ins, ops, m: str) -> str:
        kind = m[-2:]
        stem = m[3:-2]
        if stem in CMP_SUFFIX:
            pred = CMP_SUFFIX[stem]
        elif stem == "" and len(ops) == 3:
            pred = ops[2].imm & 7
        else:
            raise SseUnsupported(m)
        d = self.dst(ins, ops[0])
        size = {"ss": 4, "sd": 8, "ps": 16, "pd": 16}[kind]
        if kind in ("ss", "ps"):
            lanes = range(1 if kind == "ss" else 4)
            body = " ".join(f"{d}.d[{k}] = sse_cmp({pred}, {d}.f[{k}], s.f[{k}]) ? 0xFFFFFFFFu : 0;"
                            for k in lanes)
        else:
            lanes = range(1 if kind == "sd" else 2)
            body = " ".join(f"{d}.q[{k}] = sse_cmp({pred}, {d}.g[{k}], s.g[{k}]) ? ~0ull : 0;"
                            for k in lanes)
        return self.block(self.load(ins, ops[1], size), body)

    def comis(self, ins, ops, lane: str, size: int) -> str:
        n = _xreq(ins, ops[0])
        return self.block(self.load(ins, ops[1], size),
                          f"sse_comis(c, c->xmm[{n}].{lane}[0], s.{lane}[0]);")

    def op_comiss(self, ins, ops): return self.comis(ins, ops, "f", 4)
    op_ucomiss = op_comiss

    def op_comisd(self, ins, ops): return self.comis(ins, ops, "g", 8)
    op_ucomisd = op_comisd

    # -- conversions ----------------------------------------------------------------------
    def op_cvtsi2ss(self, ins, ops):
        return f"c->xmm[{_xreq(ins, ops[0])}].f[0] = (float)(int32_t){self.L.read(ins, ops[1], 4)};"

    def op_cvtsi2sd(self, ins, ops):
        return f"c->xmm[{_xreq(ins, ops[0])}].g[0] = (double)(int32_t){self.L.read(ins, ops[1], 4)};"

    def to_int(self, ins, ops, lane: str, size: int, truncate: int) -> str:
        return self.block(self.load(ins, ops[1], size),
                          self.L.write(ins, ops[0], f"(uint32_t)sse_f2i(s.{lane}[0], {truncate})"))

    def op_cvttss2si(self, ins, ops): return self.to_int(ins, ops, "f", 4, 1)
    def op_cvtss2si(self, ins, ops): return self.to_int(ins, ops, "f", 4, 0)
    def op_cvttsd2si(self, ins, ops): return self.to_int(ins, ops, "g", 8, 1)
    def op_cvtsd2si(self, ins, ops): return self.to_int(ins, ops, "g", 8, 0)

    def op_cvtss2sd(self, ins, ops):
        return self.block(self.load(ins, ops[1], 4), f"{self.dst(ins, ops[0])}.g[0] = (double)s.f[0];")

    def op_cvtsd2ss(self, ins, ops):
        return self.block(self.load(ins, ops[1], 8), f"{self.dst(ins, ops[0])}.f[0] = (float)s.g[0];")

    def op_cvtdq2ps(self, ins, ops):
        d = self.dst(ins, ops[0])
        return self.block(self.load(ins, ops[1], 16),
                          " ".join(f"{d}.f[{k}] = (float)s.i[{k}];" for k in range(4)))

    def op_cvtdq2pd(self, ins, ops):
        d = self.dst(ins, ops[0])
        return self.block(self.load(ins, ops[1], 8),
                          f"{d}.g[0] = (double)s.i[0]; {d}.g[1] = (double)s.i[1];")

    def op_cvtps2pd(self, ins, ops):
        d = self.dst(ins, ops[0])
        return self.block(self.load(ins, ops[1], 8),
                          f"{d}.g[0] = (double)s.f[0]; {d}.g[1] = (double)s.f[1];")

    def op_cvtpd2ps(self, ins, ops):
        d = self.dst(ins, ops[0])
        return self.block(self.load(ins, ops[1], 16),
                          f"{d}.f[0] = (float)s.g[0]; {d}.f[1] = (float)s.g[1]; {d}.q[1] = 0;")

    def packed_to_int(self, ins, ops, truncate: int) -> str:
        d = self.dst(ins, ops[0])
        return self.block(self.load(ins, ops[1], 16),
                          " ".join(f"{d}.i[{k}] = sse_f2i(s.f[{k}], {truncate});" for k in range(4)))

    def op_cvttps2dq(self, ins, ops): return self.packed_to_int(ins, ops, 1)
    def op_cvtps2dq(self, ins, ops): return self.packed_to_int(ins, ops, 0)

    # -- shuffles and unpacks -----------------------------------------------------------------
    def op_shufps(self, ins, ops):
        d, imm = self.dst(ins, ops[0]), ops[2].imm
        sel = [imm & 3, imm >> 2 & 3, imm >> 4 & 3, imm >> 6 & 3]
        return self.block(self.load(ins, ops[1], 16), f"Xmm a = {d};",
                          f"{d}.d[0] = a.d[{sel[0]}]; {d}.d[1] = a.d[{sel[1]}];",
                          f"{d}.d[2] = s.d[{sel[2]}]; {d}.d[3] = s.d[{sel[3]}];")

    def op_shufpd(self, ins, ops):
        d, imm = self.dst(ins, ops[0]), ops[2].imm
        return self.block(self.load(ins, ops[1], 16), f"Xmm a = {d};",
                          f"{d}.q[0] = a.q[{imm & 1}]; {d}.q[1] = s.q[{imm >> 1 & 1}];")

    def op_pshufd(self, ins, ops):
        d, imm = self.dst(ins, ops[0]), ops[2].imm
        return self.block(self.load(ins, ops[1], 16),
                          " ".join(f"{d}.d[{k}] = s.d[{imm >> (2 * k) & 3}];" for k in range(4)))

    def op_pshuflw(self, ins, ops):
        d, imm = self.dst(ins, ops[0]), ops[2].imm
        return self.block(self.load(ins, ops[1], 16),
                          " ".join(f"{d}.w[{k}] = s.w[{imm >> (2 * k) & 3}];" for k in range(4)),
                          f"{d}.q[1] = s.q[1];")

    def op_pshufhw(self, ins, ops):
        d, imm = self.dst(ins, ops[0]), ops[2].imm
        return self.block(self.load(ins, ops[1], 16), f"{d}.q[0] = s.q[0];",
                          " ".join(f"{d}.w[{4 + k}] = s.w[{4 + (imm >> (2 * k) & 3)}];" for k in range(4)))

    def unpack(self, ins, ops, lane: str, count: int, high: bool) -> str:
        d = self.dst(ins, ops[0])
        base = count // 2 if high else 0
        body = []
        for k in range(count // 2):
            body.append(f"{d}.{lane}[{2 * k}] = a.{lane}[{base + k}];")
            body.append(f"{d}.{lane}[{2 * k + 1}] = s.{lane}[{base + k}];")
        return self.block(self.load(ins, ops[1], 16), f"Xmm a = {d};", *body)

    def op_unpcklps(self, ins, ops): return self.unpack(ins, ops, "d", 4, False)
    def op_unpckhps(self, ins, ops): return self.unpack(ins, ops, "d", 4, True)
    def op_unpcklpd(self, ins, ops): return self.unpack(ins, ops, "q", 2, False)
    def op_unpckhpd(self, ins, ops): return self.unpack(ins, ops, "q", 2, True)
    def op_punpcklbw(self, ins, ops): return self.unpack(ins, ops, "b", 16, False)
    def op_punpckhbw(self, ins, ops): return self.unpack(ins, ops, "b", 16, True)
    def op_punpcklwd(self, ins, ops): return self.unpack(ins, ops, "w", 8, False)
    def op_punpckhwd(self, ins, ops): return self.unpack(ins, ops, "w", 8, True)
    def op_punpckldq(self, ins, ops): return self.unpack(ins, ops, "d", 4, False)
    def op_punpckhdq(self, ins, ops): return self.unpack(ins, ops, "d", 4, True)
    def op_punpcklqdq(self, ins, ops): return self.unpack(ins, ops, "q", 2, False)
    def op_punpckhqdq(self, ins, ops): return self.unpack(ins, ops, "q", 2, True)

    # -- integer lanes ----------------------------------------------------------------------
    def int_lanes(self, ins, ops, kind: str, sym: str) -> str:
        count, lane, t = INT_LANES[kind]
        d = self.dst(ins, ops[0])
        utype = "u" + t
        body = " ".join(f"{d}.{lane}[{k}] = ({utype})({d}.{lane}[{k}] {sym} s.{lane}[{k}]);"
                        for k in range(count))
        return self.block(self.load(ins, ops[1], 16), body)

    def int_cmp(self, ins, ops, kind: str, sym: str) -> str:
        count, lane, t = INT_LANES[kind]
        d = self.dst(ins, ops[0])
        body = " ".join(f"{d}.{lane}[{k}] = (({t}){d}.{lane}[{k}] {sym} ({t})s.{lane}[{k}]) ? ({'u' + t})~0ull : 0;"
                        for k in range(count))
        return self.block(self.load(ins, ops[1], 16), body)

    def shift(self, ins, ops, m: str) -> str:
        kind = m[-1]
        count, lane, t = INT_LANES[kind]
        bits = 128 // count
        d = self.dst(ins, ops[0])
        if ops[1].type == X.X86_OP_IMM:
            pre, amount = "", f"{ops[1].imm & 0xFF}u"
        else:
            pre, amount = self.load(ins, ops[1], 16), "(s.q[0] > 255 ? 255u : (unsigned)s.q[0])"
        if m.startswith("psra"):
            expr = lambda k: f"({'u' + t})(({t}){d}.{lane}[{k}] >> (n > {bits - 1} ? {bits - 1} : n))"
        elif m.startswith("psll"):
            expr = lambda k: f"n > {bits - 1} ? 0 : ({'u' + t})({d}.{lane}[{k}] << n)"
        else:
            expr = lambda k: f"n > {bits - 1} ? 0 : ({'u' + t})({d}.{lane}[{k}] >> n)"
        body = " ".join(f"{d}.{lane}[{k}] = {expr(k)};" for k in range(count))
        return self.block(pre, f"unsigned n = {amount};", body)

    def op_psrldq(self, ins, ops):
        d, n = self.dst(ins, ops[0]), min(ops[1].imm, 16)
        return self.block(f"Xmm a = {d};", f"memset(&{d}, 0, 16);",
                          f"memcpy(&{d}.b[0], &a.b[{n}], {16 - n});" if n < 16 else "")

    def op_pslldq(self, ins, ops):
        d, n = self.dst(ins, ops[0]), min(ops[1].imm, 16)
        return self.block(f"Xmm a = {d};", f"memset(&{d}, 0, 16);",
                          f"memcpy(&{d}.b[{n}], &a.b[0], {16 - n});" if n < 16 else "")

    def op_pmaddwd(self, ins, ops):
        d = self.dst(ins, ops[0])
        body = " ".join(f"r.i[{k}] = (int32_t){d}.sw[{2 * k}] * s.sw[{2 * k}] + "
                        f"(int32_t){d}.sw[{2 * k + 1}] * s.sw[{2 * k + 1}];" for k in range(4))
        return self.block(self.load(ins, ops[1], 16), "Xmm r;", body, f"{d} = r;")

    def op_pmulhw(self, ins, ops):
        d = self.dst(ins, ops[0])
        body = " ".join(f"{d}.w[{k}] = (uint16_t)(((int32_t){d}.sw[{k}] * s.sw[{k}]) >> 16);"
                        for k in range(8))
        return self.block(self.load(ins, ops[1], 16), body)

    def op_pmullw(self, ins, ops):
        d = self.dst(ins, ops[0])
        body = " ".join(f"{d}.w[{k}] = (uint16_t)((int32_t){d}.sw[{k}] * s.sw[{k}]);" for k in range(8))
        return self.block(self.load(ins, ops[1], 16), body)

    def op_packssdw(self, ins, ops):
        d = self.dst(ins, ops[0])
        body = [f"r.sw[{k}] = sse_sat16(a.i[{k}]);" for k in range(4)]
        body += [f"r.sw[{k + 4}] = sse_sat16(s.i[{k}]);" for k in range(4)]
        return self.block(self.load(ins, ops[1], 16), f"Xmm a = {d}, r;", *body, f"{d} = r;")

    def op_packuswb(self, ins, ops):
        d = self.dst(ins, ops[0])
        body = [f"r.b[{k}] = sse_satu8(a.sw[{k}]);" for k in range(8)]
        body += [f"r.b[{k + 8}] = sse_satu8(s.sw[{k}]);" for k in range(8)]
        return self.block(self.load(ins, ops[1], 16), f"Xmm a = {d}, r;", *body, f"{d} = r;")

    def op_pextrw(self, ins, ops):
        n = _xreq(ins, ops[1])
        return self.L.write(ins, ops[0], f"(uint32_t)c->xmm[{n}].w[{ops[2].imm & 7}]")

    def op_pinsrw(self, ins, ops):
        return f"c->xmm[{_xreq(ins, ops[0])}].w[{ops[2].imm & 7}] = (uint16_t){self.L.read(ins, ops[1], 2)};"

    # -- state ------------------------------------------------------------------------------
    def op_ldmxcsr(self, ins, ops):
        return f"c->mxcsr = rd32({self.L.address(ins, ops[0])});"

    def op_stmxcsr(self, ins, ops):
        return f"wr32({self.L.address(ins, ops[0])}, c->mxcsr);"

    def op_emms(self, ins, ops):
        return ";"

/* Guest CPU state and helpers used by C code generated from th123.exe.
 *
 * The guest is 32-bit x86. Guest addresses are 32-bit values; the host reaches
 * them through guest_base (G()). Integer flags are computed eagerly into single
 * bytes. The x87 FPU keeps an explicit control word: precision control (24/53/64
 * significand bits) and rounding control are applied by the runtime on every
 * operation instead of relying on the host FPU mode.
 */
#ifndef T123_GUEST_H
#define T123_GUEST_H

#include <setjmp.h>
#include <stdint.h>
#include <string.h>

#if defined(__x86_64__) || defined(__i386__)
typedef long double x87_t;   /* 64-bit significand on x86 hosts */
#define X87_HAS_EXTENDED 1
#else
typedef double x87_t;        /* ARM: PC=64 degrades to 53 bits (documented) */
#define X87_HAS_EXTENDED 0
#endif

typedef union {
    uint32_t d;
    uint16_t w;
    struct { uint8_t l, h; } b;
} GuestReg;

typedef struct {
    x87_t st[8];
    uint16_t cw;   /* control word: PC bits 8-9, RC bits 10-11 */
    uint16_t sw;   /* status word without TOP; C0 bit 8, C1 9, C2 10, C3 14 */
    uint8_t top;
    uint8_t tag_empty; /* bitmask of empty physical registers */
} X87;

/* SSE register: viewed as bytes, words, dwords, qwords, floats or doubles. */
typedef union {
    uint8_t b[16];
    uint16_t w[8];
    int16_t sw[8];
    uint32_t d[4];
    int32_t i[4];
    uint64_t q[2];
    float f[4];
    double g[2];
} Xmm;

typedef struct Cpu {
    GuestReg eax, ecx, edx, ebx, esp, ebp, esi, edi;
    uint8_t cf, zf, sf, of, pf, df;
    uint8_t id_flag; /* EFLAGS.ID, toggled by the CRT to probe for cpuid */
    uint32_t fs_base;
    X87 fpu;
    Xmm xmm[8];
    uint32_t mxcsr; /* kept for ldmxcsr/stmxcsr; conversions use round-to-nearest */
} Cpu;

/* Fixed Windows user-mode selectors, for code that stores segment registers. */
enum { SEL_CS = 0x1B, SEL_DS = 0x23, SEL_ES = 0x23, SEL_SS = 0x23, SEL_FS = 0x3B, SEL_GS = 0x00 };

typedef void (*GuestFn)(Cpu *c);

/* Debug builds (-DT123_TRACE_CALLS) remember the last guest functions entered, so a
   crash can be placed in the original code. */
/* Per-thread runtime state. VitaSDK's _Thread_local gives every thread the same
   address (measured in Vita3K), so the Vita keeps it behind a pthread key. */
/* C++ catch continuations: a lifted function that contains one records its host frame on
   entry (guest esp, function); _JumpToContinuation (host/seh.c) longjmps back there with the
   target address in seh_resume_target. Records of returned functions are dropped lazily. */
typedef struct { uint32_t fn, esp; jmp_buf *jb; } SehResume;
#define SEH_RESUME_MAX 256
typedef struct {
    uint32_t current_target;   /* address of the call being dispatched */
    uint32_t resume_target;
    unsigned resume_n;
    SehResume resumes[SEH_RESUME_MAX];
    uint32_t trace_ring[64];
    unsigned trace_pos;
    jmp_buf *fault_trap;
    char fault_text[256];      /* last guest_fault, for reports */
} GuestThread;
#ifdef __vita__
GuestThread *guest_thread(void);
#else
extern _Thread_local GuestThread guest_thread_state;
#define guest_thread() (&guest_thread_state)
#endif
#define guest_trace_ring (guest_thread()->trace_ring)
#define guest_trace_pos (guest_thread()->trace_pos)
#define guest_current_target (guest_thread()->current_target)
#define guest_fault_trap (guest_thread()->fault_trap)
#define guest_fault_text (guest_thread()->fault_text)
#define seh_resume_target (guest_thread()->resume_target)
void seh_resume_push(Cpu *c, uint32_t fn, jmp_buf *jb);
#ifdef T123_TRACE_CALLS
#define T123_ENTER(a) (guest_trace_ring[guest_trace_pos++ & 63] = (a))
#elif defined(T123_PROFILE)
/* Profile builds: the last guest function entered, sampled by host/profile.c. */
extern volatile uint32_t guest_prof_last;
#define T123_ENTER(a) (guest_prof_last = (a))
#else
#define T123_ENTER(a) ((void)0)
#endif

/* -- memory ------------------------------------------------------------------------ */
/* Guest addresses below GUEST_WINDOW are backed by host memory. On PC the host
   reserves the whole 4 GiB range (so stray accesses fault); on the Vita it is a
   single block of GUEST_WINDOW bytes. */
/* Vagante keeps every decoded image in the guest heap: 1 GiB on PC; the Vita build sets
   its own size (its memory is the limit there). */
#ifndef GUEST_WINDOW
#define GUEST_WINDOW 0x40000000u
#endif
extern uint8_t *guest_base;
#define G(a) (guest_base + (uint32_t)(a))

int guest_mem_init(void);
extern int guest_mem_error; /* platform error code when guest_mem_init fails */
int guest_map(uint32_t address, uint32_t size);
int guest_load_pe(const char *path); /* copies headers and sections at ImageBase */
extern uint32_t guest_image_base, guest_image_size;

static inline uint8_t rd8(uint32_t a) { return *G(a); }
static inline uint16_t rd16(uint32_t a) { uint16_t v; memcpy(&v, G(a), 2); return v; }
static inline uint32_t rd32(uint32_t a) { uint32_t v; memcpy(&v, G(a), 4); return v; }
static inline uint64_t rd64(uint32_t a) { uint64_t v; memcpy(&v, G(a), 8); return v; }
static inline void wr8(uint32_t a, uint8_t v) { *G(a) = v; }
static inline void wr16(uint32_t a, uint16_t v) { memcpy(G(a), &v, 2); }
static inline void wr32(uint32_t a, uint32_t v) { memcpy(G(a), &v, 4); }
static inline void wr64(uint32_t a, uint64_t v) { memcpy(G(a), &v, 8); }

/* -- SSE ------------------------------------------------------------------------------- */
/* Load n bytes into the low part of a register value; the rest is zero (as movss/movsd
   /movq from memory do). */
static inline Xmm xld(uint32_t a, unsigned n) { Xmm v; memset(&v, 0, 16); memcpy(&v, G(a), n); return v; }
static inline void xst(uint32_t a, const Xmm *v, unsigned n) { memcpy(G(a), v, n); }
static inline float rdf32(uint32_t a) { float v; memcpy(&v, G(a), 4); return v; }
static inline double rdf64(uint32_t a) { double v; memcpy(&v, G(a), 8); return v; }

/* float/double -> int32 as cvt(t)ss2si: out of range or NaN gives 0x80000000. */
static inline int32_t sse_f2i(double v, int truncate) {
    if (!(v > -2147483649.0 && v < 2147483648.0)) return INT32_MIN;
    double r = truncate ? (v < 0 ? -__builtin_floor(-v) : __builtin_floor(v)) : __builtin_nearbyint(v);
    if (r < -2147483648.0 || r > 2147483647.0) return INT32_MIN;
    return (int32_t)r;
}

/* (u)comiss/(u)comisd: ZF,PF,CF from the comparison; OF=SF=0. */
static inline void sse_comis(Cpu *c, double a, double b) {
    c->of = c->sf = 0;
    if (a != a || b != b) { c->zf = c->pf = c->cf = 1; return; }
    c->pf = 0;
    c->zf = a == b;
    c->cf = a < b;
}

/* cmpXXss/sd/ps/pd predicate imm8 (0 eq, 1 lt, 2 le, 3 unord, 4 neq, 5 nlt, 6 nle, 7 ord). */
static inline int sse_cmp(int pred, double a, double b) {
    int unord = a != a || b != b;
    switch (pred & 7) {
    case 0: return !unord && a == b;
    case 1: return !unord && a < b;
    case 2: return !unord && a <= b;
    case 3: return unord;
    case 4: return unord || a != b;
    case 5: return unord || !(a < b);
    case 6: return unord || !(a <= b);
    default: return !unord;
    }
}
/* add/sub/mul/div: when an operand is NaN, x86 returns the first NaN operand, quieted.
   C may evaluate a commutative operation with the operands swapped, so the payload
   is chosen explicitly. -DSSE_FAST_NAN drops the checks (payload then unspecified). */
#ifdef SSE_FAST_NAN
#define SSE_ARITH_F(a, op, b) ((a) op (b))
#define SSE_ARITH_D(a, op, b) ((a) op (b))
#else
static inline float sse_qnanf(float v) { uint32_t u; memcpy(&u, &v, 4); u |= 0x00400000u; memcpy(&v, &u, 4); return v; }
static inline double sse_qnand(double v) { uint64_t u; memcpy(&u, &v, 8); u |= 0x0008000000000000ull; memcpy(&v, &u, 8); return v; }
#define SSE_ARITH_F(a, op, b) ({ float a_ = (a), b_ = (b); \
    a_ != a_ ? sse_qnanf(a_) : b_ != b_ ? sse_qnanf(b_) : a_ op b_; })
#define SSE_ARITH_D(a, op, b) ({ double a_ = (a), b_ = (b); \
    a_ != a_ ? sse_qnand(a_) : b_ != b_ ? sse_qnand(b_) : a_ op b_; })
#endif

/* minss/maxss return the second operand when either is NaN or both are zero. */
static inline float sse_minf(float a, float b) { return a < b ? a : b; }
static inline float sse_maxf(float a, float b) { return a > b ? a : b; }
static inline double sse_mind(double a, double b) { return a < b ? a : b; }
static inline double sse_maxd(double a, double b) { return a > b ? a : b; }
static inline int16_t sse_sat16(int32_t v) { return v > 32767 ? 32767 : v < -32768 ? -32768 : (int16_t)v; }
static inline uint8_t sse_satu8(int32_t v) { return v > 255 ? 255 : v < 0 ? 0 : (uint8_t)v; }

static inline void push32(Cpu *c, uint32_t v) { c->esp.d -= 4; wr32(c->esp.d, v); }
static inline uint32_t pop32(Cpu *c) { uint32_t v = rd32(c->esp.d); c->esp.d += 4; return v; }

/* -- calls ------------------------------------------------------------------------- */
typedef struct { uint32_t address; GuestFn fn; } GuestEntry;

void guest_register(const GuestEntry *table, unsigned count); /* lifted functions */
void guest_intercept(uint32_t address, GuestFn fn);           /* host replacements */
void guest_call(Cpu *c, uint32_t target);  /* return address already pushed */
void guest_call_import(Cpu *c, uint32_t iat_slot);
/* Host -> guest call (callbacks such as window procedures and thread entries):
   pushes argv right to left and a sentinel return address, returns eax. */
#define GUEST_RETURN_SENTINEL 0xFFFFFFF0u
uint32_t guest_invoke(Cpu *c, uint32_t target, unsigned argc, const uint32_t *argv);
/* Reports and exits, or longjmps to guest_fault_trap when a test harness set one. */
_Noreturn void guest_fault(Cpu *c, uint32_t address, const char *what);

/* -- integer flags ------------------------------------------------------------------ */
static inline uint8_t parity8(uint32_t r) { return (uint8_t)!__builtin_parity(r & 0xFF); }

#define T123_DEFINE_FLAGS(N, T, ST, MSB)                                                    \
static inline void flags_logic##N(Cpu *c, T r) {                                            \
    c->cf = 0; c->of = 0; c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r);     \
}                                                                                            \
static inline T op_add##N(Cpu *c, T a, T b) {                                                \
    T r = (T)(a + b);                                                                        \
    c->cf = r < a; c->of = (((a ^ r) & (b ^ r)) >> (MSB)) & 1;                               \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_adc##N(Cpu *c, T a, T b) {                                                \
    T carry = c->cf; T r = (T)(a + b + carry);                                               \
    c->cf = carry ? r <= a : r < a; c->of = (((a ^ r) & (b ^ r)) >> (MSB)) & 1;               \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_sub##N(Cpu *c, T a, T b) {                                                \
    T r = (T)(a - b);                                                                        \
    c->cf = a < b; c->of = (((a ^ b) & (a ^ r)) >> (MSB)) & 1;                               \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_sbb##N(Cpu *c, T a, T b) {                                                \
    T borrow = c->cf; T r = (T)(a - b - borrow);                                             \
    c->cf = borrow ? a <= b : a < b; c->of = (((a ^ b) & (a ^ r)) >> (MSB)) & 1;             \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_inc##N(Cpu *c, T a) {                                                     \
    T r = (T)(a + 1); c->of = r == (T)((T)1 << (MSB));                                       \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_dec##N(Cpu *c, T a) {                                                     \
    T r = (T)(a - 1); c->of = a == (T)((T)1 << (MSB));                                       \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_neg##N(Cpu *c, T a) {                                                     \
    T r = (T)(0 - a); c->cf = a != 0; c->of = a == (T)((T)1 << (MSB));                       \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_shl##N(Cpu *c, T a, uint8_t n) {                                          \
    n &= 31; if (!n) return a;                                                               \
    T r = n > (MSB) ? 0 : (T)(a << n);                                                       \
    c->cf = n <= (MSB) + 1 ? (a >> ((MSB) + 1 - n)) & 1 : 0;                                 \
    c->of = ((r >> (MSB)) & 1) ^ c->cf;                                                      \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_shr##N(Cpu *c, T a, uint8_t n) {                                          \
    n &= 31; if (!n) return a;                                                               \
    T r = n > (MSB) ? 0 : (T)(a >> n);                                                       \
    c->cf = n <= (MSB) + 1 ? (a >> (n - 1)) & 1 : 0;                                         \
    c->of = (a >> (MSB)) & 1;                                                                \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}                                                                                            \
static inline T op_sar##N(Cpu *c, T a, uint8_t n) {                                          \
    n &= 31; if (!n) return a;                                                               \
    ST s = (ST)a; uint8_t k = n > (MSB) ? (MSB) : n;                                         \
    T r = (T)(s >> k);                                                                       \
    c->cf = (uint8_t)(((ST)a >> (n > (MSB) ? (MSB) : n - 1)) & 1);                           \
    c->of = 0;                                                                               \
    c->zf = r == 0; c->sf = (r >> (MSB)) & 1; c->pf = parity8(r); return r;                  \
}

T123_DEFINE_FLAGS(8, uint8_t, int8_t, 7)
T123_DEFINE_FLAGS(16, uint16_t, int16_t, 15)
T123_DEFINE_FLAGS(32, uint32_t, int32_t, 31)

#define T123_DEFINE_ROTATES(N, T, BITS)                                                      \
static inline T op_rol##N(Cpu *c, T a, uint8_t n) {                                          \
    n &= 31; if (!n) return a;                                                               \
    uint8_t k = n % (BITS); T r = k ? (T)((a << k) | (a >> ((BITS) - k))) : a;               \
    c->cf = r & 1; c->of = ((r >> ((BITS) - 1)) & 1) ^ c->cf; return r;                      \
}                                                                                            \
static inline T op_ror##N(Cpu *c, T a, uint8_t n) {                                          \
    n &= 31; if (!n) return a;                                                               \
    uint8_t k = n % (BITS); T r = k ? (T)((a >> k) | (a << ((BITS) - k))) : a;               \
    c->cf = (r >> ((BITS) - 1)) & 1;                                                         \
    c->of = ((r >> ((BITS) - 1)) ^ (r >> ((BITS) - 2))) & 1; return r;                       \
}                                                                                            \
static inline T op_rcl##N(Cpu *c, T a, uint8_t n) {                                          \
    n = (uint8_t)((n & 31) % ((BITS) + 1)); if (!n) return a;                                \
    for (uint8_t i = 0; i < n; i++) {                                                        \
        uint8_t out = (a >> ((BITS) - 1)) & 1; a = (T)((a << 1) | c->cf); c->cf = out;       \
    }                                                                                        \
    c->of = ((a >> ((BITS) - 1)) & 1) ^ c->cf; return a;                                     \
}                                                                                            \
static inline T op_rcr##N(Cpu *c, T a, uint8_t n) {                                          \
    n = (uint8_t)((n & 31) % ((BITS) + 1)); if (!n) return a;                                \
    c->of = ((a >> ((BITS) - 1)) & 1) ^ c->cf; /* defined for n == 1 */                      \
    for (uint8_t i = 0; i < n; i++) {                                                        \
        uint8_t out = a & 1; a = (T)((a >> 1) | ((T)c->cf << ((BITS) - 1))); c->cf = out;    \
    }                                                                                        \
    return a;                                                                                \
}

T123_DEFINE_ROTATES(8, uint8_t, 8)
T123_DEFINE_ROTATES(16, uint16_t, 16)
T123_DEFINE_ROTATES(32, uint32_t, 32)

/* shld/shrd r/m32, r32, count. */
static inline uint32_t op_shld32(Cpu *c, uint32_t a, uint32_t b, uint8_t n) {
    n &= 31; if (!n) return a;
    uint32_t r = (a << n) | (b >> (32 - n));
    c->cf = (a >> (32 - n)) & 1; c->of = ((r ^ a) >> 31) & 1;
    c->zf = r == 0; c->sf = r >> 31; c->pf = parity8(r); return r;
}
static inline uint32_t op_shrd32(Cpu *c, uint32_t a, uint32_t b, uint8_t n) {
    n &= 31; if (!n) return a;
    uint32_t r = (a >> n) | (b << (32 - n));
    c->cf = (a >> (n - 1)) & 1; c->of = ((r ^ a) >> 31) & 1;
    c->zf = r == 0; c->sf = r >> 31; c->pf = parity8(r); return r;
}

/* imul r/m8: ax = al * src (signed). */
static inline void op_imul1_8(Cpu *c, uint8_t b) {
    int16_t p = (int16_t)((int8_t)c->eax.b.l * (int8_t)b);
    c->eax.w = (uint16_t)p;
    c->cf = c->of = p != (int8_t)p;
    c->zf = (uint8_t)p == 0; c->sf = ((uint16_t)p >> 7) & 1; c->pf = parity8((uint8_t)p);
}

/* xadd: dst = dst + src, src = old dst; `lock` makes the memory form atomic. */
static inline uint32_t op_xadd32_mem(Cpu *c, uint32_t ea, uint32_t src) {
    uint32_t old = __atomic_fetch_add((uint32_t *)G(ea), src, __ATOMIC_SEQ_CST);
    (void)op_add32(c, old, src);
    return old;
}

/* EFLAGS image for pushfd: CF PF ZF SF DF OF, IF and reserved bit 1 set, ID kept. */
static inline uint32_t get_eflags(Cpu *c) {
    return (uint32_t)c->cf | 2u | (uint32_t)c->pf << 2 | (uint32_t)c->zf << 6 |
           (uint32_t)c->sf << 7 | 1u << 9 | (uint32_t)c->df << 10 | (uint32_t)c->of << 11 |
           (uint32_t)c->id_flag << 21;
}
static inline void set_eflags(Cpu *c, uint32_t f) {
    c->cf = f & 1; c->pf = (f >> 2) & 1; c->zf = (f >> 6) & 1; c->sf = (f >> 7) & 1;
    c->df = (f >> 10) & 1; c->of = (f >> 11) & 1; c->id_flag = (f >> 21) & 1;
}

/* bt/bts with a register bit offset: for memory operands the offset addresses a
   bit string (signed, may leave the dword). */
static inline uint32_t bt_address(uint32_t ea, uint32_t offset) {
    return ea + (uint32_t)(((int32_t)offset >> 5) * 4);
}

void op_cpuid(Cpu *c);
void op_scas(Cpu *c, unsigned size, int rep, uint32_t address);  /* rep: 0, 1 repe, 2 repne */
void op_cmps(Cpu *c, unsigned size, int rep, uint32_t address);

/* imul r, r/m(, imm): truncated product; CF=OF when the signed result overflowed. */
static inline uint32_t op_imul32(Cpu *c, uint32_t a, uint32_t b) {
    int64_t p = (int64_t)(int32_t)a * (int32_t)b;
    uint32_t r = (uint32_t)p;
    c->cf = c->of = p != (int32_t)r;
    c->zf = r == 0; c->sf = r >> 31; c->pf = parity8(r); return r;
}
static inline uint16_t op_imul16(Cpu *c, uint16_t a, uint16_t b) {
    int32_t p = (int32_t)(int16_t)a * (int16_t)b;
    uint16_t r = (uint16_t)p;
    c->cf = c->of = p != (int16_t)r;
    c->zf = r == 0; c->sf = r >> 15; c->pf = parity8(r); return r;
}
/* One-operand forms on edx:eax. */
static inline void op_imul1_32(Cpu *c, uint32_t b) {
    int64_t p = (int64_t)(int32_t)c->eax.d * (int32_t)b;
    c->eax.d = (uint32_t)p; c->edx.d = (uint32_t)((uint64_t)p >> 32);
    c->cf = c->of = p != (int32_t)(uint32_t)p;
    c->zf = c->eax.d == 0; c->sf = c->eax.d >> 31; c->pf = parity8(c->eax.d);
}
static inline void op_mul1_32(Cpu *c, uint32_t b) {
    uint64_t p = (uint64_t)c->eax.d * b;
    c->eax.d = (uint32_t)p; c->edx.d = (uint32_t)(p >> 32);
    c->cf = c->of = c->edx.d != 0;
    c->zf = c->eax.d == 0; c->sf = c->eax.d >> 31; c->pf = parity8(c->eax.d);
}
void op_div1_32(Cpu *c, uint32_t b, uint32_t address);
void op_idiv1_32(Cpu *c, uint32_t b, uint32_t address);

/* Condition codes. */
#define CC_O(c)  ((c)->of)
#define CC_NO(c) (!(c)->of)
#define CC_B(c)  ((c)->cf)
#define CC_AE(c) (!(c)->cf)
#define CC_E(c)  ((c)->zf)
#define CC_NE(c) (!(c)->zf)
#define CC_BE(c) ((c)->cf | (c)->zf)
#define CC_A(c)  (!((c)->cf | (c)->zf))
#define CC_S(c)  ((c)->sf)
#define CC_NS(c) (!(c)->sf)
#define CC_P(c)  ((c)->pf)
#define CC_NP(c) (!(c)->pf)
#define CC_L(c)  ((c)->sf != (c)->of)
#define CC_GE(c) ((c)->sf == (c)->of)
#define CC_LE(c) ((c)->zf | ((c)->sf != (c)->of))
#define CC_G(c)  (!(c)->zf && (c)->sf == (c)->of)

/* sahf: SF ZF AF PF CF from AH (AF is not modelled). */
static inline void op_sahf(Cpu *c) {
    uint8_t ah = c->eax.b.h;
    c->sf = (ah >> 7) & 1; c->zf = (ah >> 6) & 1; c->pf = (ah >> 2) & 1; c->cf = ah & 1;
}

/* String operations; DF=1 steps backwards. */
void op_rep_movs(Cpu *c, unsigned size, int rep, uint32_t address);
void op_rep_stos(Cpu *c, unsigned size, int rep, uint32_t address);

/* -- x87 ----------------------------------------------------------------------------- */
enum { X87_ADD, X87_SUB, X87_SUBR, X87_MUL, X87_DIV, X87_DIVR };

void x87_reset(Cpu *c);                     /* fninit: CW 0x037F, empty stack */
void x87_set_cw(Cpu *c, uint16_t cw);       /* fldcw */
static inline uint16_t x87_get_cw(Cpu *c) { return c->fpu.cw; }
uint16_t x87_get_sw(Cpu *c);                /* fnstsw, with TOP */

x87_t x87_st(Cpu *c, unsigned i);
void x87_set_st(Cpu *c, unsigned i, x87_t v);
void x87_push(Cpu *c, x87_t v);
void x87_pop(Cpu *c);

void x87_fld_m32(Cpu *c, uint32_t a);
void x87_fld_m64(Cpu *c, uint32_t a);
void x87_fld_m80(Cpu *c, uint32_t a);
void x87_fild(Cpu *c, int64_t v);
void x87_fld_const(Cpu *c, int which);      /* 0 zero, 1 one, 2 pi, 3 l2e, 4 l2t, 5 lg2, 6 ln2 */
void x87_fld_st(Cpu *c, unsigned i);
void x87_fst_m32(Cpu *c, uint32_t a, int pop);
void x87_fst_m64(Cpu *c, uint32_t a, int pop);
void x87_fst_m80(Cpu *c, uint32_t a, int pop);
void x87_fst_st(Cpu *c, unsigned i, int pop);
void x87_fist(Cpu *c, uint32_t a, unsigned size, int pop, int truncate);
void x87_arith(Cpu *c, int op, unsigned dst, x87_t src, int pop); /* st(dst) = st(dst) op src */
x87_t x87_m32(uint32_t a);
x87_t x87_m64(uint32_t a);
void x87_compare(Cpu *c, x87_t b, int pops, int unordered_ok);   /* st0 vs b -> C0 C2 C3 */
void x87_comi(Cpu *c, unsigned i, int pop);                      /* fcomi/fucomi -> EFLAGS */
void x87_fxch(Cpu *c, unsigned i);
void x87_unary(Cpu *c, int which); /* 0 chs 1 abs 2 sqrt 3 rndint 4 sin 5 cos 6 tst 7 xam */
void x87_binary_special(Cpu *c, int which); /* 0 fpatan 1 fyl2x 2 fscale 3 fprem 4 f2xm1 5 fptan 6 fprem1 */
void x87_fnstenv(Cpu *c, uint32_t a);
void x87_fldenv(Cpu *c, uint32_t a);
void x87_fnsave(Cpu *c, uint32_t a);
void x87_frstor(Cpu *c, uint32_t a);
x87_t x87_round_pc(Cpu *c, x87_t v);        /* apply PC with the current RC */

#endif

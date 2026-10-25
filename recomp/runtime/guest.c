/* Host side of the guest runtime: memory, PE loading, call dispatch, faults,
 * integer helpers that need control flow, and the explicit-state x87 model. */
#include "guest.h"

#include <fenv.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#if defined(__vita__)
#include <psp2/kernel/sysmem.h>
#elif defined(_WIN32)
#include <windows.h>
#else
#include <sys/mman.h>
#endif

uint8_t *guest_base;
uint32_t guest_image_base, guest_image_size;
int guest_mem_error;

/* -- memory ---------------------------------------------------------------------------- */
int guest_mem_init(void) {
    if (guest_base)
        return 0;
#if defined(__vita__)
    /* One block for the whole guest window; zeroed like freshly committed pages. */
    SceUID block = sceKernelAllocMemBlock("t123 guest", SCE_KERNEL_MEMBLOCK_TYPE_USER_RW,
                                          GUEST_WINDOW, NULL);
    void *p = NULL;
    if (block < 0) {
        guest_mem_error = block;
        return -1;
    }
    if ((guest_mem_error = sceKernelGetMemBlockBase(block, &p)) < 0)
        return -1;
    memset(p, 0, GUEST_WINDOW);
    guest_base = p;
#elif defined(_WIN32)
    guest_base = VirtualAlloc(NULL, (SIZE_T)1 << 32, MEM_RESERVE, PAGE_NOACCESS);
#else
    void *p = mmap(NULL, (size_t)1 << 32, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);
    guest_base = p == MAP_FAILED ? NULL : p;
#endif
    return guest_base ? 0 : -1;
}

int guest_map(uint32_t address, uint32_t size) {
    uint32_t lo = address & ~0xFFFu;
    uint64_t hi = ((uint64_t)address + size + 0xFFF) & ~(uint64_t)0xFFF;
#if defined(__vita__)
    (void)lo;
    return hi <= GUEST_WINDOW ? 0 : -1; /* the window is allocated up front */
#elif defined(_WIN32)
    return VirtualAlloc(G(lo), (SIZE_T)(hi - lo), MEM_COMMIT, PAGE_READWRITE) ? 0 : -1;
#else
    return mprotect(G(lo), (size_t)(hi - lo), PROT_READ | PROT_WRITE);
#endif
}

int guest_load_pe(const char *path) {
    FILE *f = fopen(path, "rb");
    if (!f)
        return -1;
    fseek(f, 0, SEEK_END);
    long size = ftell(f);
    fseek(f, 0, SEEK_SET);
    uint8_t *file = malloc((size_t)size);
    if (!file || fread(file, 1, (size_t)size, f) != (size_t)size) {
        fclose(f);
        free(file);
        return -1;
    }
    fclose(f);
    uint32_t pe, image_base, image_size, headers;
    uint16_t sections, optional;
    memcpy(&pe, file + 0x3C, 4);
    memcpy(&sections, file + pe + 6, 2);
    memcpy(&optional, file + pe + 20, 2);
    memcpy(&image_base, file + pe + 24 + 28, 4);
    memcpy(&image_size, file + pe + 24 + 56, 4);
    memcpy(&headers, file + pe + 24 + 60, 4);
    if (guest_map(image_base, image_size)) {
        free(file);
        return -1;
    }
    guest_image_base = image_base;
    guest_image_size = image_size;
    memcpy(G(image_base), file, headers);
    const uint8_t *table = file + pe + 24 + optional;
    for (unsigned i = 0; i < sections; i++) {
        uint32_t vsize, va, raw_size, raw_ptr;
        memcpy(&vsize, table + i * 40 + 8, 4);
        memcpy(&va, table + i * 40 + 12, 4);
        memcpy(&raw_size, table + i * 40 + 16, 4);
        memcpy(&raw_ptr, table + i * 40 + 20, 4);
        uint32_t copy = raw_size < vsize || !vsize ? raw_size : vsize;
        memcpy(G(image_base + va), file + raw_ptr, copy);
    }
    free(file);
    return 0;
}

/* -- calls ----------------------------------------------------------------------------- */
static const GuestEntry *lifted;
static unsigned lifted_count;
static GuestEntry intercepts[2048]; /* kept sorted by address */
static unsigned intercept_count;

void guest_register(const GuestEntry *table, unsigned count) {
    lifted = table;
    lifted_count = count;
}

static GuestFn search(const GuestEntry *table, unsigned count, uint32_t target) {
    unsigned lo = 0, hi = count;
    while (lo < hi) {
        unsigned mid = (lo + hi) / 2;
        if (table[mid].address == target)
            return table[mid].fn;
        if (table[mid].address < target)
            lo = mid + 1;
        else
            hi = mid;
    }
    return NULL;
}

void guest_intercept(uint32_t address, GuestFn fn) {
    unsigned i = 0;
    while (i < intercept_count && intercepts[i].address < address)
        i++;
    if (i < intercept_count && intercepts[i].address == address) {
        intercepts[i].fn = fn; /* replace */
        return;
    }
    if (intercept_count == sizeof intercepts / sizeof intercepts[0]) {
        fprintf(stderr, "guest: intercept table full\n");
        exit(3);
    }
    memmove(&intercepts[i + 1], &intercepts[i], (intercept_count - i) * sizeof intercepts[0]);
    intercepts[i] = (GuestEntry){address, fn};
    intercept_count++;
}

static GuestFn find(uint32_t target) {
    GuestFn fn = intercept_count ? search(intercepts, intercept_count, target) : NULL;
    return fn ? fn : search(lifted, lifted_count, target);
}

#ifdef __vita__
#include <pthread.h>
#include <stdlib.h>
static pthread_key_t thread_key;
static pthread_once_t thread_once = PTHREAD_ONCE_INIT;
static void thread_key_init(void) { pthread_key_create(&thread_key, free); }
GuestThread *guest_thread(void) {
    pthread_once(&thread_once, thread_key_init);
    GuestThread *t = pthread_getspecific(thread_key);
    if (!t) {
        t = calloc(1, sizeof *t);
        pthread_setspecific(thread_key, t);
    }
    return t;
}
#else
_Thread_local GuestThread guest_thread_state;
#endif

void guest_call(Cpu *c, uint32_t target) {
    GuestFn fn = find(target);
    if (!fn)
        guest_fault(c, target, "call to a function that is neither lifted nor intercepted");
    guest_current_target = target;
    fn(c);
}

/* Imports go through the IAT contents, as in the original: the host fills every
 * slot with an address it intercepts, so `call [slot]` and pointers loaded from
 * the IAT reach the same implementation. Without a filled IAT the slot address
 * itself is looked up (unit-test harnesses register intercepts that way). */
void guest_call_import(Cpu *c, uint32_t iat_slot) {
    GuestFn fn = find(iat_slot);
    if (fn) {
        guest_current_target = iat_slot;
        fn(c);
        return;
    }
    guest_call(c, rd32(iat_slot));
}

uint32_t guest_invoke(Cpu *c, uint32_t target, unsigned argc, const uint32_t *argv) {
    uint32_t esp = c->esp.d;
    for (unsigned i = argc; i-- > 0;)
        push32(c, argv[i]);
    push32(c, GUEST_RETURN_SENTINEL);
    guest_call(c, target);
    c->esp.d = esp; /* works for both cdecl and stdcall callbacks */
    return c->eax.d;
}


_Noreturn void guest_fault(Cpu *c, uint32_t address, const char *what) {
    snprintf(guest_fault_text, sizeof guest_fault_text, "guest_fault at 0x%08x: %s", address, what);
    fprintf(stderr, "%s\n"
            "  eax=%08x ecx=%08x edx=%08x ebx=%08x esp=%08x ebp=%08x esi=%08x edi=%08x\n",
            guest_fault_text, c->eax.d, c->ecx.d, c->edx.d, c->ebx.d, c->esp.d, c->ebp.d,
            c->esi.d, c->edi.d);
    if (guest_fault_trap)
        longjmp(*guest_fault_trap, 1);
    exit(3);
}

/* -- integer helpers ------------------------------------------------------------------- */
void op_div1_32(Cpu *c, uint32_t b, uint32_t address) {
    uint64_t n = ((uint64_t)c->edx.d << 32) | c->eax.d;
    if (!b || n / b > 0xFFFFFFFFu)
        guest_fault(c, address, "#DE in div");
    c->eax.d = (uint32_t)(n / b);
    c->edx.d = (uint32_t)(n % b);
}

void op_idiv1_32(Cpu *c, uint32_t b, uint32_t address) {
    int64_t n = (int64_t)(((uint64_t)c->edx.d << 32) | c->eax.d);
    int32_t d = (int32_t)b;
    if (!d || (n == INT64_MIN && d == -1))
        guest_fault(c, address, "#DE in idiv");
    int64_t q = n / d;
    if (q > INT32_MAX || q < INT32_MIN)
        guest_fault(c, address, "#DE in idiv (quotient overflow)");
    c->eax.d = (uint32_t)(int32_t)q;
    c->edx.d = (uint32_t)(int32_t)(n % d);
}

/* String instructions step by +size, or by -size when DF=1 (std). */
void op_rep_movs(Cpu *c, unsigned size, int rep, uint32_t address) {
    (void)address;
    uint32_t count = rep ? c->ecx.d : 1;
    if (!c->df) {
        /* Forward, element by element semantics: overlapping copies replicate. */
        if (c->edi.d <= c->esi.d || c->edi.d >= c->esi.d + count * size) {
            memmove(G(c->edi.d), G(c->esi.d), (size_t)count * size);
        } else {
            for (uint32_t i = 0; i < count * size; i++)
                wr8(c->edi.d + i, rd8(c->esi.d + i));
        }
        c->esi.d += count * size;
        c->edi.d += count * size;
    } else {
        for (uint32_t i = 0; i < count; i++) {
            memmove(G(c->edi.d), G(c->esi.d), size);
            c->esi.d -= size;
            c->edi.d -= size;
        }
    }
    if (rep)
        c->ecx.d = 0;
}

void op_rep_stos(Cpu *c, unsigned size, int rep, uint32_t address) {
    (void)address;
    uint32_t count = rep ? c->ecx.d : 1;
    int32_t step = c->df ? -(int32_t)size : (int32_t)size;
    for (uint32_t i = 0; i < count; i++) {
        if (size == 1)
            wr8(c->edi.d, c->eax.b.l);
        else if (size == 2)
            wr16(c->edi.d, c->eax.w);
        else
            wr32(c->edi.d, c->eax.d);
        c->edi.d += (uint32_t)step;
    }
    if (rep)
        c->ecx.d = 0;
}

/* cpuid: a Pentium Pro class CPU with FPU, TSC, CX8 and CMOV but no MMX/SSE/SSE2.
 * The MSVC CRT then keeps its x87 code paths (__sse2_available stays 0), which the
 * runtime models and validates; its SSE2 math routines are never entered. */
void op_cpuid(Cpu *c) {
    switch (c->eax.d) {
    case 0:
        c->eax.d = 1;
        c->ebx.d = 0x756E6547; c->edx.d = 0x49656E69; c->ecx.d = 0x6C65746E; /* GenuineIntel */
        break;
    case 1:
        c->eax.d = 0x00000633; c->ebx.d = 0; c->ecx.d = 0;
        c->edx.d = 1u << 0 | 1u << 4 | 1u << 8 | 1u << 15;
        break;
    default:
        c->eax.d = c->ebx.d = c->ecx.d = c->edx.d = 0;
        break;
    }
}

static uint32_t string_value(Cpu *c, unsigned size, uint32_t a) {
    return size == 1 ? rd8(a) : size == 2 ? rd16(a) : rd32(a);
}

static void string_compare(Cpu *c, unsigned size, uint32_t a, uint32_t b) {
    if (size == 1) (void)op_sub8(c, (uint8_t)a, (uint8_t)b);
    else if (size == 2) (void)op_sub16(c, (uint16_t)a, (uint16_t)b);
    else (void)op_sub32(c, a, b);
}

void op_scas(Cpu *c, unsigned size, int rep, uint32_t address) {
    (void)address;
    uint32_t acc = size == 1 ? c->eax.b.l : size == 2 ? c->eax.w : c->eax.d;
    uint32_t step = c->df ? (uint32_t)-(int32_t)size : size;
    do {
        if (rep && c->ecx.d == 0)
            break;
        string_compare(c, size, acc, string_value(c, size, c->edi.d));
        c->edi.d += step;
        if (rep)
            c->ecx.d--;
    } while (rep && (rep == 1 ? c->zf : !c->zf));
}

void op_cmps(Cpu *c, unsigned size, int rep, uint32_t address) {
    (void)address;
    uint32_t step = c->df ? (uint32_t)-(int32_t)size : size;
    do {
        if (rep && c->ecx.d == 0)
            break;
        string_compare(c, size, string_value(c, size, c->esi.d), string_value(c, size, c->edi.d));
        c->esi.d += step;
        c->edi.d += step;
        if (rep)
            c->ecx.d--;
    } while (rep && (rep == 1 ? c->zf : !c->zf));
}

/* -- x87 ---------------------------------------------------------------------------------
 * Values live in x87_t. Precision control (PC) is applied to the result of every
 * arithmetic operation, using the rounding control (RC) of the guest control word:
 *   PC=24: the operation is computed in double and rounded to a 24-bit significand
 *          (round24: exponent kept, no denormalisation). Double rounding through 53
 *          bits is innocuous for + - * / sqrt of float-precision operands because
 *          53 >= 2*24+2, and for directed modes because the 24-bit grid is a subset
 *          of the 53-bit one.
 *   PC=53: the operation is computed in double.
 *   PC=64: computed in x87_t; on hosts without an 80-bit type this degrades to 53 bits.
 * The extended exponent range of x87 registers is not modelled.
 */
static const int rc_to_fenv[4] = {FE_TONEAREST, FE_DOWNWARD, FE_UPWARD, FE_TOWARDZERO};

#define RC(c) (((c)->fpu.cw >> 10) & 3)
#define PC(c) (((c)->fpu.cw >> 8) & 3)

/* Runs `expr` under the guest rounding mode; the fast path avoids touching the host FPU. */
#define WITH_RC(c, decl, expr)                                       \
    do {                                                            \
        int rc_ = RC(c);                                            \
        if (rc_ == 0) {                                             \
            decl = (expr);                                          \
        } else {                                                    \
            int saved_ = fegetround();                              \
            fesetround(rc_to_fenv[rc_]);                            \
            decl = (expr);                                          \
            fesetround(saved_);                                     \
        }                                                           \
    } while (0)

void x87_reset(Cpu *c) {
    memset(&c->fpu, 0, sizeof c->fpu);
    c->fpu.cw = 0x037F;
    c->fpu.tag_empty = 0xFF;
}

void x87_set_cw(Cpu *c, uint16_t cw) { c->fpu.cw = cw; }

uint16_t x87_get_sw(Cpu *c) {
    return (uint16_t)((c->fpu.sw & ~0x3800) | ((c->fpu.top & 7) << 11));
}

static inline unsigned phys(Cpu *c, unsigned i) { return (c->fpu.top + i) & 7; }

x87_t x87_st(Cpu *c, unsigned i) { return c->fpu.st[phys(c, i)]; }
void x87_set_st(Cpu *c, unsigned i, x87_t v) {
    unsigned p = phys(c, i);
    c->fpu.st[p] = v;
    c->fpu.tag_empty &= (uint8_t)~(1u << p);
}

void x87_push(Cpu *c, x87_t v) {
    c->fpu.top = (c->fpu.top - 1) & 7;
    unsigned p = c->fpu.top;
    if (!(c->fpu.tag_empty & (1u << p))) {
        c->fpu.sw |= 0x0241; /* stack fault: IE, SF, C1=1 (overflow) */
        v = (x87_t)NAN;      /* masked response: indefinite */
    }
    c->fpu.st[p] = v;
    c->fpu.tag_empty &= (uint8_t)~(1u << p);
}

void x87_pop(Cpu *c) {
    c->fpu.tag_empty |= (uint8_t)(1u << c->fpu.top);
    c->fpu.top = (c->fpu.top + 1) & 7;
}

static float to_float_rc(Cpu *c, x87_t v) {
    volatile float r;
    WITH_RC(c, r, (float)v);
    return r;
}

static double to_double_rc(Cpu *c, x87_t v) {
    volatile double r;
    WITH_RC(c, r, (double)v);
    return r;
}

/* PC=24 rounds the significand to 24 bits but keeps the extended exponent of the
 * register: no denormalisation and no overflow to infinity. Scaling by a power of
 * two is exact, so rounding the scaled value to float is rounding to 24 bits. */
static double round24(Cpu *c, double v) {
    if (v == 0 || !isfinite(v))
        return to_float_rc(c, v);
    int e;
    frexp(v, &e);
    return ldexp((double)to_float_rc(c, ldexp(v, -e)), e);
}

x87_t x87_round_pc(Cpu *c, x87_t v) {
    switch (PC(c)) {
    case 0: return round24(c, (double)v);
    case 2: return to_double_rc(c, v);
    default: return v;
    }
}

x87_t x87_m32(uint32_t a) { float f; memcpy(&f, G(a), 4); return f; }
x87_t x87_m64(uint32_t a) { double d; memcpy(&d, G(a), 8); return d; }

void x87_fld_m32(Cpu *c, uint32_t a) { x87_push(c, x87_m32(a)); }
void x87_fld_m64(Cpu *c, uint32_t a) { x87_push(c, x87_m64(a)); }

static long double read80(uint32_t a);
void x87_fld_m80(Cpu *c, uint32_t a) { x87_push(c, (x87_t)read80(a)); }

void x87_fild(Cpu *c, int64_t v) { x87_push(c, (x87_t)v); }

void x87_fld_const(Cpu *c, int which) {
    static const long double k[] = {
        0.0L, 1.0L, 3.14159265358979323846264338327950288L, 1.44269504088896340735992468100189214L,
        3.32192809488736234787031942948939018L, 0.301029995663981195213738894724493027L,
        0.693147180559945309417232121458176568L};
    x87_push(c, (x87_t)k[which]);
}

void x87_fld_st(Cpu *c, unsigned i) { x87_push(c, x87_st(c, i)); }

void x87_fst_m32(Cpu *c, uint32_t a, int pop) {
    float f = to_float_rc(c, x87_st(c, 0));
    memcpy(G(a), &f, 4);
    if (pop) x87_pop(c);
}

void x87_fst_m64(Cpu *c, uint32_t a, int pop) {
    double d = to_double_rc(c, x87_st(c, 0));
    memcpy(G(a), &d, 8);
    if (pop) x87_pop(c);
}

static void write80(uint32_t a, long double v) {
    uint16_t se = signbit(v) ? 0x8000 : 0;
    uint64_t mant = 0;
    if (isnan(v)) {
        se |= 0x7FFF; mant = 0xC000000000000000ull;
    } else if (isinf(v)) {
        se |= 0x7FFF; mant = 0x8000000000000000ull;
    } else if (v != 0) {
        int e;
        long double m = frexpl(fabsl(v), &e); /* v = m * 2^e, 0.5 <= m < 1 */
        mant = (uint64_t)ldexpl(m, 64);
        se |= (uint16_t)(e - 1 + 16383);
    }
    wr64(a, mant);
    wr16(a + 8, se);
}

static long double read80(uint32_t a) {
    uint64_t mant = rd64(a);
    uint16_t se = rd16(a + 8);
    int exp = se & 0x7FFF;
    long double v;
    if (exp == 0x7FFF)
        v = (mant << 1) ? (long double)NAN : (long double)INFINITY;
    else
        v = ldexpl((long double)mant, exp - 16383 - 63);
    return (se & 0x8000) ? -v : v;
}

void x87_fst_m80(Cpu *c, uint32_t a, int pop) {
    write80(a, x87_st(c, 0));
    if (pop) x87_pop(c);
}

/* 32-bit protected-mode environment (28 bytes): CW, SW, TW, then pointers (zero).
 * Tags are 00 (valid) or 11 (empty); the zero/special distinction is not modelled. */
static uint16_t tag_word(Cpu *c) {
    uint16_t tw = 0;
    for (unsigned p = 0; p < 8; p++)
        if (c->fpu.tag_empty & (1u << p))
            tw |= (uint16_t)(3u << (2 * p));
    return tw;
}

void x87_fnstenv(Cpu *c, uint32_t a) {
    memset(G(a), 0, 28);
    wr32(a, c->fpu.cw | 0xFFFF0000u);
    wr32(a + 4, x87_get_sw(c) | 0xFFFF0000u);
    wr32(a + 8, tag_word(c) | 0xFFFF0000u);
    c->fpu.cw |= 0x3F; /* fnstenv masks all exceptions */
}

void x87_fldenv(Cpu *c, uint32_t a) {
    c->fpu.cw = rd16(a);
    uint16_t sw = rd16(a + 4), tw = rd16(a + 8);
    c->fpu.sw = sw & ~0x3800;
    c->fpu.top = (sw >> 11) & 7;
    c->fpu.tag_empty = 0;
    for (unsigned p = 0; p < 8; p++)
        if (((tw >> (2 * p)) & 3) == 3)
            c->fpu.tag_empty |= (uint8_t)(1u << p);
}

void x87_fnsave(Cpu *c, uint32_t a) {
    x87_fnstenv(c, a);
    for (unsigned i = 0; i < 8; i++)
        write80(a + 28 + 10 * i, x87_st(c, i));
    x87_reset(c);
}

void x87_frstor(Cpu *c, uint32_t a) {
    x87_fldenv(c, a);
    for (unsigned i = 0; i < 8; i++)
        c->fpu.st[phys(c, i)] = (x87_t)read80(a + 28 + 10 * i);
}

void x87_fst_st(Cpu *c, unsigned i, int pop) {
    x87_set_st(c, i, x87_st(c, 0));
    if (pop) x87_pop(c);
}

void x87_fist(Cpu *c, uint32_t a, unsigned size, int pop, int truncate) {
    x87_t v = x87_st(c, 0);
    long double r;
    if (truncate)
        r = truncl(v);
    else
        WITH_RC(c, r, nearbyintl(v));
    int64_t lo = size == 2 ? INT16_MIN : size == 4 ? INT32_MIN : INT64_MIN;
    int64_t hi = size == 2 ? INT16_MAX : size == 4 ? INT32_MAX : INT64_MAX;
    int64_t out;
    if (isnan(r) || r < (long double)lo || r > (long double)hi) {
        c->fpu.sw |= 0x0001; /* IE; masked response is the integer indefinite */
        out = lo;
    } else {
        out = (int64_t)r;
    }
    if (size == 2) wr16(a, (uint16_t)out);
    else if (size == 4) wr32(a, (uint32_t)out);
    else wr64(a, (uint64_t)out);
    if (pop) x87_pop(c);
}

static x87_t apply(Cpu *c, int op, x87_t a, x87_t b) {
    int pc = PC(c);
    if (pc == 3 || pc == 1) {
        volatile x87_t r;
        switch (op) {
        case X87_ADD: WITH_RC(c, r, a + b); break;
        case X87_SUB: WITH_RC(c, r, a - b); break;
        case X87_SUBR: WITH_RC(c, r, b - a); break;
        case X87_MUL: WITH_RC(c, r, a * b); break;
        case X87_DIV: WITH_RC(c, r, a / b); break;
        default: WITH_RC(c, r, b / a); break;
        }
        return r;
    }
    double da = (double)a, db = (double)b;
    volatile double r;
    switch (op) {
    case X87_ADD: WITH_RC(c, r, da + db); break;
    case X87_SUB: WITH_RC(c, r, da - db); break;
    case X87_SUBR: WITH_RC(c, r, db - da); break;
    case X87_MUL: WITH_RC(c, r, da * db); break;
    case X87_DIV: WITH_RC(c, r, da / db); break;
    default: WITH_RC(c, r, db / da); break;
    }
    return pc == 0 ? round24(c, r) : r;
}

void x87_arith(Cpu *c, int op, unsigned dst, x87_t src, int pop) {
    x87_set_st(c, dst, apply(c, op, x87_st(c, dst), src));
    if (pop) x87_pop(c);
}

void x87_compare(Cpu *c, x87_t b, int pops, int unordered_ok) {
    x87_t a = x87_st(c, 0);
    uint16_t sw = c->fpu.sw & ~0x4700;
    if (isnan(a) || isnan(b)) {
        sw |= 0x4500;
        if (!unordered_ok) sw |= 0x0001;
    } else if (a < b) {
        sw |= 0x0100;
    } else if (a == b) {
        sw |= 0x4000;
    }
    c->fpu.sw = sw;
    while (pops-- > 0) x87_pop(c);
}

void x87_comi(Cpu *c, unsigned i, int pop) {
    x87_t a = x87_st(c, 0), b = x87_st(c, i);
    if (isnan(a) || isnan(b)) { c->zf = c->pf = c->cf = 1; }
    else { c->zf = a == b; c->pf = 0; c->cf = a < b; }
    c->of = c->sf = 0;
    if (pop) x87_pop(c);
}

void x87_fxch(Cpu *c, unsigned i) {
    x87_t a = x87_st(c, 0);
    x87_set_st(c, 0, x87_st(c, i));
    x87_set_st(c, i, a);
}

void x87_unary(Cpu *c, int which) {
    x87_t a = x87_st(c, 0), r = a;
    switch (which) {
    case 0: r = -a; break;
    case 1: r = fabsl(a); break;
    case 2: {
        if (PC(c) == 3 || PC(c) == 1) { volatile x87_t t; WITH_RC(c, t, sqrtl(a)); r = t; }
        else { volatile double t; WITH_RC(c, t, sqrt((double)a)); r = PC(c) == 0 ? round24(c, t) : t; }
        break;
    }
    case 3: { volatile x87_t t; WITH_RC(c, t, nearbyintl(a)); r = t; break; }
    case 4: r = x87_round_pc(c, sinl(a)); break;
    case 5: r = x87_round_pc(c, cosl(a)); break;
    case 6: x87_compare(c, 0, 0, 0); return; /* ftst */
    case 7: {                                /* fxam */
        uint16_t sw = c->fpu.sw & ~0x4700;
        if (signbit(a)) sw |= 0x0200;
        if (c->fpu.tag_empty & (1u << phys(c, 0))) sw |= 0x4100;
        else if (isnan(a)) sw |= 0x0100;
        else if (isinf(a)) sw |= 0x0500;
        else if (a == 0) sw |= 0x4000;
        else if (fpclassify(a) == FP_SUBNORMAL) sw |= 0x4400;
        else sw |= 0x0400;
        c->fpu.sw = sw;
        return;
    }
    }
    x87_set_st(c, 0, r);
}

void x87_binary_special(Cpu *c, int which) {
    x87_t a = x87_st(c, 0), b = x87_st(c, 1);
    switch (which) {
    case 0: x87_set_st(c, 1, x87_round_pc(c, atan2l(b, a))); x87_pop(c); break;      /* fpatan */
    case 1: x87_set_st(c, 1, x87_round_pc(c, b * log2l(a))); x87_pop(c); break;      /* fyl2x */
    case 2: x87_set_st(c, 0, ldexpl(a, (int)truncl(b))); break;                      /* fscale */
    case 3: x87_set_st(c, 0, fmodl(a, b)); c->fpu.sw &= ~0x0400; break;              /* fprem */
    case 6: x87_set_st(c, 0, remainderl(a, b)); c->fpu.sw &= ~0x0400; break;         /* fprem1 */
    case 4: /* f2xm1; expm1 avoids the cancellation of exp2(x) - 1 for small x */
        x87_set_st(c, 0, x87_round_pc(c, expm1l(a * 0.693147180559945309417232121458176568L)));
        break;
    default: x87_set_st(c, 0, x87_round_pc(c, tanl(a))); x87_push(c, 1); break;      /* fptan */
    }
}

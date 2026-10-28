/* C++ exceptions for vagante.exe: x86 frame-based SEH, enough for MSVC throw/catch.
 *
 * The guest's own EH runtime does the work (it is lifted): _CxxThrowException calls
 * RaiseException, whose dispatch here calls every fs:[0] registration's handler, normally the
 * function's stub into __CxxFrameHandler3. A handler that finds a catch calls RtlUnwind (the
 * destructors of the frames above), runs the catch funclet and leaves through
 * _JumpToContinuation, which on Windows reloads ebp/esp and jumps into the middle of the
 * function that caught. Here that jump is a longjmp to the setjmp the lifter put at the entry
 * of that function (lift/lifter.py resume points, analysis/eh.py continuations), followed by
 * a goto to the continuation label.
 *
 * Not supported: __except blocks (_except_handler4 transfers control by `jmp eax` without
 * _JumpToContinuation) and continuing an exception from a filter. */
#include "win32.h"
#include <string.h>

#define JUMP_TO_CONTINUATION 0x008D5EC2u /* _JumpToContinuation(target, pRN) (PDB) */
enum { EH_NONCONTINUABLE = 1, EH_UNWINDING = 2, EH_EXIT_UNWIND = 4, CONTEXT_SIZE = 0x2CC,
       CXX_EXCEPTION = 0xE06D7363u, STATUS_UNWIND = 0xC0000027u };

extern const uint32_t seh_resume_map[][2];
extern const unsigned seh_resume_count;

/* Name of the thrown C++ type (first catchable type), for the trace. */
static const char *cxx_type(uint32_t rec) {
    if (rd32(rec) != CXX_EXCEPTION || rd32(rec + 16) < 3) return "";
    uint32_t info = rd32(rec + 20 + 8), types = info ? rd32(info + 12) : 0;
    uint32_t first = types && rd32(types) ? rd32(types + 4) : 0, td = first ? rd32(first + 4) : 0;
    return td ? STR(td + 8) : "";
}

/* Scratch area below esp: EXCEPTION_RECORD, CONTEXT and a dispatcher context. */
static uint32_t scratch(Cpu *c, uint32_t *ctx, uint32_t *disp) {
    uint32_t rec = (c->esp.d - 0x200u) & ~15u;
    *ctx = rec - ((CONTEXT_SIZE + 15) & ~15u);
    *disp = *ctx - 16;
    for (uint32_t a = *disp; a < rec + 80; a += 4) wr32(a, 0);
    wr32(*ctx, 0x10007u); /* CONTEXT_FULL */
    wr32(*ctx + 0xB4, c->ebp.d);
    wr32(*ctx + 0xC4, c->esp.d);
    c->esp.d = *disp - 64;
    return rec;
}

static uint32_t call_handler(Cpu *c, uint32_t reg, uint32_t rec, uint32_t ctx, uint32_t disp) {
    uint32_t argv[4] = {rec, reg, ctx, disp};
    return guest_invoke(c, rd32(reg + 4), 4, argv);
}

/* RaiseException(code, flags, nargs, args) */
void seh_raise(Cpu *c) {
    uint32_t code = ARG(c, 0), flags = ARG(c, 1), n = ARG(c, 2), args = ARG(c, 3);
    uint32_t esp0 = c->esp.d, ebx = c->ebx.d, esi = c->esi.d, edi = c->edi.d, ebp = c->ebp.d;
    uint32_t ctx, disp, rec = scratch(c, &ctx, &disp);
    if (n > 15) n = 15;
    wr32(rec, code);
    wr32(rec + 4, flags & EH_NONCONTINUABLE);
    wr32(rec + 12, rd32(esp0)); /* exception address: the caller */
    wr32(rec + 16, n);
    for (uint32_t i = 0; i < n; i++) wr32(rec + 20 + 4 * i, rd32(args + 4 * i));
    wr32(ctx + 0xB8, rd32(esp0));
    trace("RaiseException(code=%08x %s) on thread %u", code, cxx_type(rec), rd32(c->fs_base + 0x24));
    for (uint32_t reg = rd32(c->fs_base); reg && reg != 0xFFFFFFFFu; reg = rd32(reg)) {
        if (call_handler(c, reg, rec, ctx, disp) == 0 && !(flags & EH_NONCONTINUABLE)) {
            /* ExceptionContinueExecution */
            c->ebx.d = ebx; c->esi.d = esi; c->edi.d = edi; c->ebp.d = ebp;
            c->esp.d = esp0;
            ret_stdcall(c, 0, 4);
            return;
        }
    }
    trace("# unhandled exception %08x %s", code, cxx_type(rec));
    trace("# last guest functions entered on this thread (oldest first):");
    for (unsigned k = 0; k < 64; k++) {
        uint32_t f = guest_trace_ring[(guest_trace_pos + k) & 63];
        if (f) trace("#   %08x", f);
    }
    static char why[160];
    snprintf(why, sizeof why, "unhandled exception %08x %s", code, cxx_type(rec));
    host_exit(c, code, why);
}

/* RtlUnwind(target_frame, target_ip, record, return_value): calls the handlers of every
   registration above target_frame with EH_UNWINDING and removes them from fs:[0]. */
void seh_rtl_unwind(Cpu *c) {
    uint32_t target = ARG(c, 0), rec = ARG(c, 2), value = ARG(c, 3);
    uint32_t esp0 = c->esp.d, ebx = c->ebx.d, esi = c->esi.d, edi = c->edi.d, ebp = c->ebp.d;
    uint32_t ctx, disp, own = scratch(c, &ctx, &disp);
    if (!rec) {
        rec = own;
        wr32(rec, STATUS_UNWIND);
        wr32(rec + 12, rd32(esp0));
    }
    wr32(rec + 4, rd32(rec + 4) | EH_UNWINDING | (target ? 0 : EH_EXIT_UNWIND));
    for (uint32_t reg = rd32(c->fs_base); reg && reg != 0xFFFFFFFFu && reg != target;
         reg = rd32(c->fs_base)) {
        call_handler(c, reg, rec, ctx, disp);
        wr32(c->fs_base, rd32(reg));
    }
    c->ebx.d = ebx; c->esi.d = esi; c->edi.d = edi; c->ebp.d = ebp;
    c->esp.d = esp0;
    ret_stdcall(c, value, 4);
}

/* _JumpToContinuation(target, pRN): pops the top registration, sets ebp = pRN + 12 and
   esp = [pRN - 4] as the original does, then resumes the function that contains target. */
static void jump_to_continuation(Cpu *c) {
    uint32_t target = ARG(c, 0), rn = ARG(c, 1);
    wr32(c->fs_base, rd32(rd32(c->fs_base)));
    c->ebp.d = rn + 12;
    c->esp.d = rd32(rn - 4);
    GuestThread *t = guest_thread();
    int best = -1;
    for (unsigned i = 0; i < t->resume_n; i++) {
        SehResume *r = &t->resumes[i];
        if (r->esp <= c->esp.d || (best >= 0 && r->esp >= t->resumes[best].esp)) continue;
        for (unsigned k = 0; k < seh_resume_count; k++)
            if (seh_resume_map[k][0] == target && seh_resume_map[k][1] == r->fn) { best = (int)i; break; }
    }
    if (best < 0) guest_fault(c, target, "catch continuation without a live resume point");
    SehResume chosen = t->resumes[best];
    unsigned n = 0; /* the deeper records belong to the frames just unwound */
    for (unsigned i = 0; i < t->resume_n; i++)
        if (t->resumes[i].esp >= chosen.esp) t->resumes[n++] = t->resumes[i];
    t->resume_n = n;
    trace("# catch: resuming %08x in %08x", target, chosen.fn);
    seh_resume_target = target;
    longjmp(*chosen.jb, 1);
}

__attribute__((constructor)) static void seh_init(void) {
    guest_intercept(JUMP_TO_CONTINUATION, jump_to_continuation);
}

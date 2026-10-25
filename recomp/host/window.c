/* USER32 windows, GDI32 pixel formats and WGL for vagante.exe, on SDL2.
 *
 * SFML (linked into the game) creates one visible window (CreateWindowExW, class
 * "SFML_Window") and, for its shared OpenGL context, hidden helper windows
 * (CreateWindowExA "STATIC"). All of them map to a single SDL window with one OpenGL
 * context: every wglCreateContext returns a token for it and wglMakeCurrent binds it.
 * The game's window procedure receives the Win32 messages SFML reacts to, built from
 * SDL events in PeekMessageW and delivered by DispatchMessageW. sf::Keyboard reads keys
 * with GetAsyncKeyState, answered from SDL's keyboard state. */
#include "win32.h"

#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define API(name) static void w_##name(Cpu *c) /* prefixed: windows.h declares the real ones */

enum { HWND_BASE = 0x00010010u, MAX_WINDOWS = 8, HDC_OFFSET = 0x100, HGLRC = 0x00060001u,
       FAKE_CURSOR = 0x00030030u, FAKE_ICON = 0x00030040u };
enum { WM_CREATE = 0x0001, WM_SIZE = 0x0005, WM_SETFOCUS = 0x0007, WM_KILLFOCUS = 0x0008,
       WM_CLOSE = 0x0010, WM_KEYDOWN = 0x0100, WM_KEYUP = 0x0101, WM_CHAR = 0x0102,
       WM_MOUSEMOVE = 0x0200, WM_LBUTTONDOWN = 0x0201, WM_LBUTTONUP = 0x0202,
       WM_RBUTTONDOWN = 0x0204, WM_RBUTTONUP = 0x0205, WM_MBUTTONDOWN = 0x0207,
       WM_MBUTTONUP = 0x0208, WM_MOUSEWHEEL = 0x020A };

typedef struct { int used, visible, w, h; uint32_t proc, longs[8]; } Window;
static Window windows[MAX_WINDOWS];
static SDL_Window *sdl_window;
static SDL_GLContext gl_context;
static uint32_t class_proc[8];
static char class_name[8][64];
static unsigned class_count;
static int focused = 1;

/* -- SDL window ------------------------------------------------------------------------------ */
static void ensure_sdl(int w, int h) {
    if (sdl_window)
        return;
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMECONTROLLER) != 0)
        trace("# SDL_Init failed: %s", SDL_GetError());
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_COMPATIBILITY);
    sdl_window = SDL_CreateWindow("Vagante", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, w > 0 ? w : 640,
                                  h > 0 ? h : 360, SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
    if (!sdl_window)
        trace("# SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_StartTextInput();
}

static Window *window_of(uint32_t hwnd) {
    unsigned i = (hwnd - HWND_BASE) / 16;
    return hwnd >= HWND_BASE && i < MAX_WINDOWS && windows[i].used ? &windows[i] : NULL;
}
static uint32_t visible_hwnd(void) {
    for (unsigned i = 0; i < MAX_WINDOWS; i++)
        if (windows[i].used && windows[i].visible) return HWND_BASE + 16 * i;
    return 0;
}

/* stdcall window procedure (hwnd, msg, wparam, lparam) of the guest. */
static uint32_t call_wndproc(Cpu *c, uint32_t proc, uint32_t hwnd, uint32_t msg, uint32_t w, uint32_t l) {
    if (!proc) return 0;
    uint32_t eax = c->eax.d, ecx = c->ecx.d, edx = c->edx.d;
    push32(c, l); push32(c, w); push32(c, msg); push32(c, hwnd);
    push32(c, GUEST_RETURN_SENTINEL);
    guest_call(c, proc);
    uint32_t r = c->eax.d;
    c->eax.d = eax; c->ecx.d = ecx; c->edx.d = edx;
    return r;
}

static uint32_t class_proc_of(const char *name) {
    for (unsigned i = 0; i < class_count; i++)
        if (!_stricmp(class_name[i], name)) return class_proc[i];
    return 0;
}

/* CreateWindowEx(ex, class, title, style, x, y, w, h, parent, menu, instance, param) */
static void create_window(Cpu *c, const char *cls, const char *title) {
    unsigned i = 0;
    while (i < MAX_WINDOWS && windows[i].used) i++;
    if (i == MAX_WINDOWS) { ret_stdcall(c, 0, 12); return; }
    Window *win = &windows[i];
    memset(win, 0, sizeof *win);
    win->used = 1;
    win->w = (int)ARG(c, 6);
    win->h = (int)ARG(c, 7);
    win->proc = class_proc_of(cls);
    uint32_t hwnd = HWND_BASE + 16 * i, param = ARG(c, 11);
    ensure_sdl(win->w, win->h);
    trace("CreateWindowEx(class=%s, title=%s, %dx%d) = %08x", cls, title, win->w, win->h, hwnd);
    if (win->proc) { /* CREATESTRUCTW: lpCreateParams first; SFML keeps it as GWLP_USERDATA */
        uint32_t cs = heap_alloc(48, 1);
        wr32(cs, param);
        wr32(cs + 24, (uint32_t)win->h); wr32(cs + 28, (uint32_t)win->w);
        call_wndproc(c, win->proc, hwnd, WM_CREATE, 0, cs);
        heap_free(cs);
    }
    ret_stdcall(c, hwnd, 12);
}
API(CreateWindowExA) {
    const char *cls = ARG(c, 1) >= 0x10000 ? STR(ARG(c, 1)) : "#atom";
    create_window(c, cls, ARG(c, 2) ? STR(ARG(c, 2)) : "");
}
static void wide_to(uint32_t s, char *out, int size) {
    int n = s >= 0x10000 ? utf16_to_mb(65001, (const uint16_t *)G(s), -1, (uint8_t *)out, size - 1, NULL) : 0;
    out[n > 0 ? n : 0] = 0;
}
API(CreateWindowExW) {
    char cls[64], title[128];
    wide_to(ARG(c, 1), cls, sizeof cls);
    wide_to(ARG(c, 2), title, sizeof title);
    create_window(c, cls, title);
}
API(RegisterClassW) { /* WNDCLASSW: lpfnWndProc +4, lpszClassName +36 */
    uint32_t wc = ARG(c, 0);
    if (class_count < 8) {
        class_proc[class_count] = rd32(wc + 4);
        wide_to(rd32(wc + 36), class_name[class_count], sizeof class_name[0]);
        trace("RegisterClassW(%s, wndproc=%08x)", class_name[class_count], class_proc[class_count]);
        class_count++;
    }
    ret_stdcall(c, 0xC000 + class_count, 1);
}
API(UnregisterClassW) { ret_stdcall(c, 1, 2); }
API(DestroyWindow) {
    Window *w = window_of(ARG(c, 0));
    if (w) w->used = 0;
    ret_stdcall(c, 1, 1);
}
API(ShowWindow) {
    Window *w = window_of(ARG(c, 0));
    int show = ARG(c, 1) != 0;
    if (w && w->proc) { /* only SFML's own window is ever shown */
        w->visible = show;
        if (sdl_window) {
            if (show) { SDL_SetWindowSize(sdl_window, w->w, w->h); SDL_ShowWindow(sdl_window); }
            else SDL_HideWindow(sdl_window);
        }
    }
    ret_stdcall(c, 0, 2);
}
API(SetWindowTextW) {
    char title[128];
    wide_to(ARG(c, 1), title, sizeof title);
    Window *w = window_of(ARG(c, 0));
    if (sdl_window && w && w->proc) SDL_SetWindowTitle(sdl_window, title);
    ret_stdcall(c, 1, 2);
}
static void resize(Window *w, int width, int height) {
    if (!w || width <= 0 || height <= 0) return;
    w->w = width; w->h = height;
    if (w->visible && sdl_window) SDL_SetWindowSize(sdl_window, width, height);
}
API(SetWindowPos) { /* (hwnd, after, x, y, cx, cy, flags): SWP_NOSIZE = 1 */
    if (!(ARG(c, 6) & 1)) resize(window_of(ARG(c, 0)), (int)ARG(c, 4), (int)ARG(c, 5));
    ret_stdcall(c, 1, 7);
}
API(MoveWindow) { resize(window_of(ARG(c, 0)), (int)ARG(c, 3), (int)ARG(c, 4)); ret_stdcall(c, 1, 6); }
static void client_size(uint32_t hwnd, int *w, int *h) {
    Window *win = window_of(hwnd);
    *w = win ? win->w : 640; *h = win ? win->h : 360;
    if (win && win->visible && sdl_window) SDL_GetWindowSize(sdl_window, w, h);
}
API(GetClientRect) {
    int w, h;
    client_size(ARG(c, 0), &w, &h);
    uint32_t r = ARG(c, 1);
    wr32(r, 0); wr32(r + 4, 0); wr32(r + 8, (uint32_t)w); wr32(r + 12, (uint32_t)h);
    ret_stdcall(c, 1, 2);
}
API(GetWindowRect) {
    int w, h, x = 0, y = 0;
    client_size(ARG(c, 0), &w, &h);
    if (sdl_window) SDL_GetWindowPosition(sdl_window, &x, &y);
    uint32_t r = ARG(c, 1);
    wr32(r, (uint32_t)x); wr32(r + 4, (uint32_t)y); wr32(r + 8, (uint32_t)(x + w)); wr32(r + 12, (uint32_t)(y + h));
    ret_stdcall(c, 1, 2);
}
API(AdjustWindowRect) { ret_stdcall(c, 1, 3); } /* no borders: client == window */
API(GetWindowLongW) {
    Window *w = window_of(ARG(c, 0));
    int32_t index = (int32_t)ARG(c, 1);
    uint32_t v = 0;
    if (w) v = index == -4 ? w->proc : (index < 0 && index >= -21) ? w->longs[(-index) % 8] : 0;
    ret_stdcall(c, v, 2);
}
API(SetWindowLongW) {
    Window *w = window_of(ARG(c, 0));
    int32_t index = (int32_t)ARG(c, 1);
    uint32_t old = 0;
    if (w) {
        if (index == -4) { old = w->proc; w->proc = ARG(c, 2); }
        else if (index < 0 && index >= -21) { old = w->longs[(-index) % 8]; w->longs[(-index) % 8] = ARG(c, 2); }
    }
    ret_stdcall(c, old, 3);
}
API(GetWindowThreadProcessId) {
    if (ARG(c, 1)) wr32(ARG(c, 1), 0x1000);
    ret_stdcall(c, 0x1004, 2);
}
API(GetForegroundWindow) { ret_stdcall(c, focused ? visible_hwnd() : 0, 0); }
API(SetForegroundWindow) { ret_stdcall(c, 1, 1); }
API(FlashWindowEx) { ret_stdcall(c, 1, 1); }
API(GetSystemMetrics) {
    SDL_DisplayMode m = {0};
    ensure_sdl(0, 0);
    SDL_GetDesktopDisplayMode(0, &m);
    uint32_t i = ARG(c, 0);
    ret_stdcall(c, i == 0 ? (uint32_t)m.w : i == 1 ? (uint32_t)m.h : 0, 1);
}
/* DEVMODEW: dmSize +68, dmFields +72, dmBitsPerPel +168, dmPelsWidth +172, dmPelsHeight +176,
   dmDisplayFrequency +184. Mode 0 and ENUM_CURRENT_SETTINGS (-1) are the desktop mode. */
API(EnumDisplaySettingsW) {
    int32_t mode = (int32_t)ARG(c, 1);
    if (mode != -1 && mode != 0) { ret_stdcall(c, 0, 3); return; }
    SDL_DisplayMode m = {0};
    ensure_sdl(0, 0);
    SDL_GetDesktopDisplayMode(0, &m);
    uint32_t d = ARG(c, 2);
    memset(G(d), 0, 220);
    wr16(d + 68, 220);
    wr32(d + 72, 0x5C0000u);
    wr32(d + 168, 32); wr32(d + 172, (uint32_t)m.w); wr32(d + 176, (uint32_t)m.h);
    wr32(d + 184, (uint32_t)(m.refresh_rate ? m.refresh_rate : 60));
    ret_stdcall(c, 1, 3);
}
API(ChangeDisplaySettingsW) { ret_stdcall(c, 0 /* DISP_CHANGE_SUCCESSFUL */, 2); }
API(SystemParametersInfoA) { if (ARG(c, 2)) memset(G(ARG(c, 2)), 0, 4); ret_stdcall(c, 1, 4); }

/* -- cursor and mouse ---------------------------------------------------------------------- */
API(LoadCursorW) { ret_stdcall(c, FAKE_CURSOR, 2); }
API(SetCursor) { ret_stdcall(c, FAKE_CURSOR, 1); }
API(CreateIcon) { ret_stdcall(c, FAKE_ICON, 7); }
API(DestroyIcon) { ret_stdcall(c, 1, 1); }
API(ClipCursor) { ret_stdcall(c, 1, 1); }
API(SetCapture) { ret_stdcall(c, 0, 1); }
API(ReleaseCapture) { ret_stdcall(c, 1, 0); }
API(GetCapture) { ret_stdcall(c, 0, 0); }
API(TrackMouseEvent) { ret_stdcall(c, 1, 1); }
API(GetCursorPos) {
    int x = 0, y = 0;
    SDL_GetGlobalMouseState(&x, &y);
    wr32(ARG(c, 0), (uint32_t)x); wr32(ARG(c, 0) + 4, (uint32_t)y);
    ret_stdcall(c, 1, 1);
}
API(ScreenToClient) {
    int x = 0, y = 0;
    if (sdl_window) SDL_GetWindowPosition(sdl_window, &x, &y);
    uint32_t p = ARG(c, 1);
    wr32(p, rd32(p) - (uint32_t)x); wr32(p + 4, rd32(p + 4) - (uint32_t)y);
    ret_stdcall(c, 1, 2);
}
API(MapWindowPoints) { /* (from, to, points, count): client of `from` -> screen when to == 0 */
    int x = 0, y = 0;
    if (sdl_window) SDL_GetWindowPosition(sdl_window, &x, &y);
    int sign = ARG(c, 1) == 0 ? 1 : ARG(c, 0) == 0 ? -1 : 0;
    for (uint32_t i = 0; i < ARG(c, 3); i++) {
        uint32_t p = ARG(c, 2) + 8 * i;
        wr32(p, rd32(p) + (uint32_t)(sign * x)); wr32(p + 4, rd32(p + 4) + (uint32_t)(sign * y));
    }
    ret_stdcall(c, (uint32_t)((sign * y) << 16 | ((sign * x) & 0xFFFF)), 4);
}

/* -- keyboard -------------------------------------------------------------------------------- */
static SDL_Scancode vk_to_scancode(uint32_t vk) {
    if (vk >= 'A' && vk <= 'Z') return (SDL_Scancode)(SDL_SCANCODE_A + (vk - 'A'));
    if (vk >= '1' && vk <= '9') return (SDL_Scancode)(SDL_SCANCODE_1 + (vk - '1'));
    if (vk == '0') return SDL_SCANCODE_0;
    if (vk >= 0x70 && vk <= 0x7B) return (SDL_Scancode)(SDL_SCANCODE_F1 + (vk - 0x70));
    if (vk >= 0x61 && vk <= 0x69) return (SDL_Scancode)(SDL_SCANCODE_KP_1 + (vk - 0x61));
    switch (vk) {
    case 0x08: return SDL_SCANCODE_BACKSPACE; case 0x09: return SDL_SCANCODE_TAB;
    case 0x0D: return SDL_SCANCODE_RETURN;    case 0x1B: return SDL_SCANCODE_ESCAPE;
    case 0x20: return SDL_SCANCODE_SPACE;     case 0x21: return SDL_SCANCODE_PAGEUP;
    case 0x22: return SDL_SCANCODE_PAGEDOWN;  case 0x23: return SDL_SCANCODE_END;
    case 0x24: return SDL_SCANCODE_HOME;      case 0x25: return SDL_SCANCODE_LEFT;
    case 0x26: return SDL_SCANCODE_UP;        case 0x27: return SDL_SCANCODE_RIGHT;
    case 0x28: return SDL_SCANCODE_DOWN;      case 0x2D: return SDL_SCANCODE_INSERT;
    case 0x2E: return SDL_SCANCODE_DELETE;    case 0x60: return SDL_SCANCODE_KP_0;
    case 0x6A: return SDL_SCANCODE_KP_MULTIPLY; case 0x6B: return SDL_SCANCODE_KP_PLUS;
    case 0x6D: return SDL_SCANCODE_KP_MINUS;  case 0x6E: return SDL_SCANCODE_KP_PERIOD;
    case 0x6F: return SDL_SCANCODE_KP_DIVIDE; case 0x13: return SDL_SCANCODE_PAUSE;
    case 0xA0: return SDL_SCANCODE_LSHIFT;    case 0xA1: return SDL_SCANCODE_RSHIFT;
    case 0xA2: return SDL_SCANCODE_LCTRL;     case 0xA3: return SDL_SCANCODE_RCTRL;
    case 0xA4: return SDL_SCANCODE_LALT;      case 0xA5: return SDL_SCANCODE_RALT;
    case 0x5B: return SDL_SCANCODE_LGUI;      case 0x5C: return SDL_SCANCODE_RGUI;
    case 0xBA: return SDL_SCANCODE_SEMICOLON; case 0xBB: return SDL_SCANCODE_EQUALS;
    case 0xBC: return SDL_SCANCODE_COMMA;     case 0xBD: return SDL_SCANCODE_MINUS;
    case 0xBE: return SDL_SCANCODE_PERIOD;    case 0xBF: return SDL_SCANCODE_SLASH;
    case 0xC0: return SDL_SCANCODE_GRAVE;     case 0xDB: return SDL_SCANCODE_LEFTBRACKET;
    case 0xDC: return SDL_SCANCODE_BACKSLASH; case 0xDD: return SDL_SCANCODE_RIGHTBRACKET;
    case 0xDE: return SDL_SCANCODE_APOSTROPHE;
    default: return SDL_SCANCODE_UNKNOWN;
    }
}
static uint32_t scancode_to_vk(SDL_Scancode s) {
    for (uint32_t vk = 1; vk < 0xFF; vk++)
        if (vk_to_scancode(vk) == s) return vk;
    return 0;
}
API(GetAsyncKeyState) {
    uint32_t vk = ARG(c, 0);
    const Uint8 *keys = SDL_GetKeyboardState(NULL);
    int down = 0;
    if (vk == 0x10) down = keys[SDL_SCANCODE_LSHIFT] || keys[SDL_SCANCODE_RSHIFT];
    else if (vk == 0x11) down = keys[SDL_SCANCODE_LCTRL] || keys[SDL_SCANCODE_RCTRL];
    else if (vk == 0x12) down = keys[SDL_SCANCODE_LALT] || keys[SDL_SCANCODE_RALT];
    else if (vk == 0x01 || vk == 0x02 || vk == 0x04) {
        Uint32 b = SDL_GetMouseState(NULL, NULL);
        down = (vk == 0x01 && (b & SDL_BUTTON_LMASK)) || (vk == 0x02 && (b & SDL_BUTTON_RMASK)) ||
               (vk == 0x04 && (b & SDL_BUTTON_MMASK));
    } else {
        SDL_Scancode s = vk_to_scancode(vk);
        down = s != SDL_SCANCODE_UNKNOWN && keys[s];
    }
    ret_stdcall(c, down && focused ? 0x8000u : 0, 1);
}
API(MapVirtualKeyW) { ret_stdcall(c, 0, 2); }

/* -- messages ---------------------------------------------------------------------------------- */
typedef struct { uint32_t hwnd, msg, w, l; } Msg;
static Msg queue[64];
static unsigned q_head, q_tail;
static void post(uint32_t msg, uint32_t w, uint32_t l) {
    uint32_t hwnd = visible_hwnd();
    if (!hwnd || q_tail - q_head >= 64) return;
    queue[q_tail++ % 64] = (Msg){hwnd, msg, w, l};
}
static uint32_t mouse_l(int x, int y) { return (uint32_t)((y & 0xFFFF) << 16 | (x & 0xFFFF)); }
static void pump(void) {
    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_QUIT: post(WM_CLOSE, 0, 0); break;
        case SDL_WINDOWEVENT:
            if (e.window.event == SDL_WINDOWEVENT_CLOSE) post(WM_CLOSE, 0, 0);
            else if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                Window *w = window_of(visible_hwnd());
                if (w) { w->w = e.window.data1; w->h = e.window.data2; }
                post(WM_SIZE, 0, mouse_l(e.window.data1, e.window.data2));
            } else if (e.window.event == SDL_WINDOWEVENT_FOCUS_GAINED) { focused = 1; post(WM_SETFOCUS, 0, 0); }
            else if (e.window.event == SDL_WINDOWEVENT_FOCUS_LOST) { focused = 0; post(WM_KILLFOCUS, 0, 0); }
            break;
        case SDL_KEYDOWN: case SDL_KEYUP: {
            uint32_t vk = scancode_to_vk(e.key.keysym.scancode);
            if (vk) post(e.type == SDL_KEYDOWN ? WM_KEYDOWN : WM_KEYUP, vk,
                         (uint32_t)e.key.keysym.scancode << 16 | (e.type == SDL_KEYUP ? 0xC0000001u : 1u));
            break;
        }
        case SDL_TEXTINPUT: {
            uint16_t wide[32];
            int n = mb_to_utf16(65001, (const uint8_t *)e.text.text, (int)strlen(e.text.text), wide, 32);
            for (int k = 0; k < n; k++) post(WM_CHAR, wide[k], 1);
            break;
        }
        case SDL_MOUSEMOTION: post(WM_MOUSEMOVE, 0, mouse_l(e.motion.x, e.motion.y)); break;
        case SDL_MOUSEBUTTONDOWN: case SDL_MOUSEBUTTONUP: {
            int up = e.type == SDL_MOUSEBUTTONUP;
            uint32_t msg = e.button.button == SDL_BUTTON_LEFT ? (up ? WM_LBUTTONUP : WM_LBUTTONDOWN)
                         : e.button.button == SDL_BUTTON_RIGHT ? (up ? WM_RBUTTONUP : WM_RBUTTONDOWN)
                         : (up ? WM_MBUTTONUP : WM_MBUTTONDOWN);
            post(msg, 0, mouse_l(e.button.x, e.button.y));
            break;
        }
        case SDL_MOUSEWHEEL: post(WM_MOUSEWHEEL, (uint32_t)(e.wheel.y * 120) << 16, 0); break;
        default: break;
        }
    }
}
/* PeekMessageW(msg, hwnd, min, max, remove): MSG is hwnd, message, wParam, lParam, time, pt. */
API(PeekMessageW) {
    if (sdl_window) pump();
    if (q_head == q_tail) { ret_stdcall(c, 0, 5); return; }
    Msg m = queue[q_head % 64];
    if (ARG(c, 4) & 1) q_head++; /* PM_REMOVE */
    uint32_t out = ARG(c, 0);
    memset(G(out), 0, 28);
    wr32(out, m.hwnd); wr32(out + 4, m.msg); wr32(out + 8, m.w); wr32(out + 12, m.l);
    wr32(out + 16, (uint32_t)(platform_ticks_us() / 1000u));
    ret_stdcall(c, 1, 5);
}
API(TranslateMessage) { ret_stdcall(c, 0, 1); } /* WM_CHAR is queued directly from SDL text input */
API(DispatchMessageW) {
    uint32_t m = ARG(c, 0);
    Window *w = window_of(rd32(m));
    uint32_t r = w ? call_wndproc(c, w->proc, rd32(m), rd32(m + 4), rd32(m + 8), rd32(m + 12)) : 0;
    ret_stdcall(c, r, 1);
}
API(SendMessageW) {
    Window *w = window_of(ARG(c, 0));
    uint32_t hwnd = ARG(c, 0), msg = ARG(c, 1), wp = ARG(c, 2), lp = ARG(c, 3);
    ret_stdcall(c, w ? call_wndproc(c, w->proc, hwnd, msg, wp, lp) : 0, 4);
}
API(CallWindowProcW) {
    uint32_t proc = ARG(c, 0), hwnd = ARG(c, 1), msg = ARG(c, 2), wp = ARG(c, 3), lp = ARG(c, 4);
    ret_stdcall(c, call_wndproc(c, proc, hwnd, msg, wp, lp), 5);
}
API(DefWindowProcW) {
    if (ARG(c, 1) == WM_CLOSE) trace("DefWindowProcW(WM_CLOSE)");
    ret_stdcall(c, 0, 4);
}

/* -- clipboard (empty) --------------------------------------------------------------------- */
API(OpenClipboard) { ret_stdcall(c, 0, 1); }
API(CloseClipboard) { ret_stdcall(c, 1, 0); }
API(EmptyClipboard) { ret_stdcall(c, 0, 0); }
API(GetClipboardData) { ret_stdcall(c, 0, 1); }
API(SetClipboardData) { ret_stdcall(c, 0, 2); }
API(IsClipboardFormatAvailable) { ret_stdcall(c, 0, 1); }

/* -- GDI32 ------------------------------------------------------------------------------------- */
API(GetDC) { ret_stdcall(c, ARG(c, 0) + HDC_OFFSET, 1); }
API(ReleaseDC) { ret_stdcall(c, 1, 2); }
API(GetDeviceCaps) { /* BITSPIXEL 12, HORZRES 8, VERTRES 10, VREFRESH 116 */
    SDL_DisplayMode m = {0};
    ensure_sdl(0, 0);
    SDL_GetDesktopDisplayMode(0, &m);
    uint32_t i = ARG(c, 1);
    ret_stdcall(c, i == 12 ? 32 : i == 8 ? (uint32_t)m.w : i == 10 ? (uint32_t)m.h : i == 116 ? 60 : 0, 2);
}
API(ChoosePixelFormat) { ret_stdcall(c, 1, 2); }
API(SetPixelFormat) { ret_stdcall(c, 1, 3); }
API(GetPixelFormat) { ret_stdcall(c, 1, 1); }
/* DescribePixelFormat(hdc, format, size, pfd): one RGBA8 double-buffered format with
   24-bit depth and 8-bit stencil. */
API(DescribePixelFormat) {
    uint32_t p = ARG(c, 3);
    if (p && ARG(c, 2) >= 40) {
        memset(G(p), 0, 40);
        wr16(p, 40); wr16(p + 2, 1);
        wr32(p + 4, 0x4 | 0x20 | 0x1);  /* DRAW_TO_WINDOW | SUPPORT_OPENGL | DOUBLEBUFFER */
        wr8(p + 8, 0);                   /* PFD_TYPE_RGBA */
        wr8(p + 9, 32);                  /* cColorBits */
        wr8(p + 10, 8); wr8(p + 11, 16); wr8(p + 12, 8); wr8(p + 13, 8); wr8(p + 14, 8); wr8(p + 15, 0);
        wr8(p + 16, 8); wr8(p + 17, 24); /* cAlphaBits, cAlphaShift */
        wr8(p + 23, 24); wr8(p + 24, 8); /* cDepthBits, cStencilBits */
    }
    ret_stdcall(c, 1, 4);
}
API(SwapBuffers) {
    if (sdl_window) SDL_GL_SwapWindow(sdl_window);
    ret_stdcall(c, 1, 1);
}

/* -- WGL ---------------------------------------------------------------------------------------- */
static int ensure_context(void) {
    ensure_sdl(0, 0);
    if (!gl_context && sdl_window) {
        gl_context = SDL_GL_CreateContext(sdl_window);
        if (!gl_context) trace("# SDL_GL_CreateContext failed: %s", SDL_GetError());
        else {
            /* glGetString without windows.h (its declarations collide with these APIs) */
            const char *(*get)(unsigned) = (const char *(*)(unsigned))SDL_GL_GetProcAddress("glGetString");
            if (get) trace("# OpenGL %s (%s)", get(0x1F02 /* GL_VERSION */), get(0x1F01 /* GL_RENDERER */));
        }
    }
    return gl_context != NULL;
}
API(wglCreateContext) { ret_stdcall(c, ensure_context() ? HGLRC : 0, 1); }
API(wglDeleteContext) { ret_stdcall(c, 1, 1); }
API(wglShareLists) { ret_stdcall(c, 1, 2); }
API(wglMakeCurrent) {
    int ok = 1;
    if (ARG(c, 1) && ensure_context()) ok = SDL_GL_MakeCurrent(sdl_window, gl_context) == 0;
    ret_stdcall(c, ok, 2);
}
/* Extensions (wglSwapIntervalEXT, framebuffer objects...) are reported absent: SFML then
   uses the OpenGL 1.1 paths the game imports directly. */
API(wglGetProcAddress) {
    trace("wglGetProcAddress(%s) = 0", STR(ARG(c, 0)));
    ret_stdcall(c, 0, 1);
}

/* -- OLE ---------------------------------------------------------------------------------------- */
API(CoCreateInstance) { if (ARG(c, 4)) wr32(ARG(c, 4), 0); ret_stdcall(c, 0x80004002u /* E_NOINTERFACE */, 5); }
API(CoSetProxyBlanket) { ret_stdcall(c, 0, 8); }
API(SysAllocString) { /* BSTR: length prefix (bytes) before the UTF-16 text */
    uint32_t s = ARG(c, 0), n = 0;
    if (!s) { ret_stdcall(c, 0, 1); return; }
    while (rd16(s + 2 * n)) n++;
    uint32_t b = heap_alloc(4 + 2 * n + 2, 1);
    wr32(b, 2 * n);
    memcpy(G(b + 4), G(s), 2 * n);
    ret_stdcall(c, b + 4, 1);
}
API(SysFreeString) { if (ARG(c, 0)) heap_free(ARG(c, 0) - 4); ret_stdcall(c, 0, 1); }

#define E(name) {#name, w_##name}
const ApiEntry win32_window[] = {
    E(CreateWindowExA), E(CreateWindowExW), E(RegisterClassW), E(UnregisterClassW), E(DestroyWindow),
    E(ShowWindow), E(SetWindowTextW), E(SetWindowPos), E(MoveWindow), E(GetClientRect), E(GetWindowRect),
    E(AdjustWindowRect), E(GetWindowLongW), E(SetWindowLongW), E(GetWindowThreadProcessId),
    E(GetForegroundWindow), E(SetForegroundWindow), E(FlashWindowEx), E(GetSystemMetrics),
    E(EnumDisplaySettingsW), E(ChangeDisplaySettingsW), E(SystemParametersInfoA),
    E(LoadCursorW), E(SetCursor), E(CreateIcon), E(DestroyIcon), E(ClipCursor), E(SetCapture),
    E(ReleaseCapture), E(GetCapture), E(TrackMouseEvent), E(GetCursorPos), E(ScreenToClient),
    E(MapWindowPoints), E(GetAsyncKeyState), E(MapVirtualKeyW),
    E(PeekMessageW), E(TranslateMessage), E(DispatchMessageW), E(SendMessageW), E(CallWindowProcW),
    E(DefWindowProcW), E(OpenClipboard), E(CloseClipboard), E(EmptyClipboard), E(GetClipboardData),
    E(SetClipboardData), E(IsClipboardFormatAvailable),
    E(GetDC), E(ReleaseDC), E(GetDeviceCaps), E(ChoosePixelFormat), E(SetPixelFormat), E(GetPixelFormat),
    E(DescribePixelFormat), E(SwapBuffers),
    E(wglCreateContext), E(wglDeleteContext), E(wglShareLists), E(wglMakeCurrent), E(wglGetProcAddress),
    E(CoCreateInstance), E(CoSetProxyBlanket), E(SysAllocString), E(SysFreeString),
    {NULL, NULL},
};

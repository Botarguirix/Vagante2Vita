#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include "win32.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
uint8_t *guest_base;
uint32_t last_error;
/* DirectInput's independent COM bridge is not exercised by this test. */
uint32_t heap_alloc(uint32_t size, int zero) { (void)size; (void)zero; return 0; }
uint32_t api_address(const char *name) { (void)name; return 0; }
static uint32_t read_pad(unsigned index, uint32_t out) {
    const ApiEntry *e = win32_stubs;
    while (strcmp(e->name, "XInputGetState")) ++e;
    Cpu c = {0}; c.esp.d = 0x100;
    wr32(0x104, index); wr32(0x108, out); e->fn(&c);
    assert(c.esp.d == 0x10c); return c.eax.d;
}
static void update(void) { SDL_PumpEvents(); SDL_GameControllerUpdate(); }
int main(void) {
    guest_base = calloc(1, 0x4000); assert(guest_base);
    assert(read_pad(0, 0x1000) == 1167); /* subsystem not initialized */
    /* Only synthetic devices: this test never depends on an attached Xbox pad. */
    SDL_SetHint("SDL_XINPUT_ENABLED", "0");
    SDL_SetHint("SDL_DIRECTINPUT_ENABLED", "0");
    SDL_SetHint("SDL_JOYSTICK_HIDAPI", "0");
    SDL_SetHint("SDL_JOYSTICK_RAWINPUT", "0");
    SDL_SetHint("SDL_JOYSTICK_WGI", "0");
    assert(SDL_Init(SDL_INIT_GAMECONTROLLER) == 0);
    assert(SDL_NumJoysticks() == 0);
    assert(read_pad(0, 0x1000) == 1167);
    assert(read_pad(4, 0x1000) == 1167 && read_pad(UINT32_MAX, 0x1000) == 1167);
    assert(read_pad(0, 0) == 87);
    int ids[4]; SDL_Joystick *joy[4];
    for (int i=0; i<4; ++i) {
        ids[i] = SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
        assert(ids[i] >= 0 && SDL_IsGameController(ids[i]));
        joy[i] = SDL_JoystickOpen(ids[i]); assert(joy[i]);
        SDL_JoystickSetVirtualAxis(joy[i], SDL_CONTROLLER_AXIS_TRIGGERLEFT, INT16_MIN);
        SDL_JoystickSetVirtualAxis(joy[i], SDL_CONTROLLER_AXIS_TRIGGERRIGHT, INT16_MIN);
    }
    update();
    memset(G(0xfff), 0xcd, 18);
    assert(read_pad(0, 0x1000) == 0);
    assert(rd8(0xfff) == 0xcd && rd8(0x1010) == 0xcd);
    uint32_t packet = rd32(0x1000); assert(packet);
    assert(read_pad(0, 0x1000) == 0 && rd32(0x1000) == packet);
    const struct { int button; unsigned mask; } buttons[] = {
        {SDL_CONTROLLER_BUTTON_DPAD_UP,1}, {SDL_CONTROLLER_BUTTON_DPAD_DOWN,2},
        {SDL_CONTROLLER_BUTTON_DPAD_LEFT,4}, {SDL_CONTROLLER_BUTTON_DPAD_RIGHT,8},
        {SDL_CONTROLLER_BUTTON_START,16}, {SDL_CONTROLLER_BUTTON_BACK,32},
        {SDL_CONTROLLER_BUTTON_LEFTSTICK,64}, {SDL_CONTROLLER_BUTTON_RIGHTSTICK,128},
        {SDL_CONTROLLER_BUTTON_LEFTSHOULDER,256}, {SDL_CONTROLLER_BUTTON_RIGHTSHOULDER,512},
        {SDL_CONTROLLER_BUTTON_A,4096}, {SDL_CONTROLLER_BUTTON_B,8192},
        {SDL_CONTROLLER_BUTTON_X,16384}, {SDL_CONTROLLER_BUTTON_Y,32768},
    };
    for (unsigned i=0; i<sizeof buttons/sizeof buttons[0]; ++i) {
        SDL_JoystickSetVirtualButton(joy[0], buttons[i].button, 1); update();
        assert(read_pad(0, 0x1000) == 0 && rd16(0x1004) == buttons[i].mask);
        assert(rd32(0x1000) > packet); packet=rd32(0x1000);
        SDL_JoystickSetVirtualButton(joy[0], buttons[i].button, 0); update();
        assert(read_pad(0, 0x1000) == 0 && rd16(0x1004) == 0);
    }
    SDL_JoystickSetVirtualAxis(joy[0], SDL_CONTROLLER_AXIS_LEFTX, INT16_MIN);
    SDL_JoystickSetVirtualAxis(joy[0], SDL_CONTROLLER_AXIS_LEFTY, INT16_MIN);
    SDL_JoystickSetVirtualAxis(joy[0], SDL_CONTROLLER_AXIS_RIGHTX, INT16_MAX);
    SDL_JoystickSetVirtualAxis(joy[0], SDL_CONTROLLER_AXIS_RIGHTY, INT16_MAX);
    SDL_JoystickSetVirtualAxis(joy[0], SDL_CONTROLLER_AXIS_TRIGGERLEFT, INT16_MAX);
    SDL_JoystickSetVirtualAxis(joy[0], SDL_CONTROLLER_AXIS_TRIGGERRIGHT, 0); update();
    assert(read_pad(0, 0x1000) == 0);
    assert(rd8(0x1006) == 255 && rd8(0x1007) >= 127 && rd8(0x1007) <= 128);
    assert((int16_t)rd16(0x1008) == INT16_MIN && (int16_t)rd16(0x100a) == INT16_MAX);
    assert((int16_t)rd16(0x100c) == INT16_MAX && (int16_t)rd16(0x100e) == -INT16_MAX);
    for (int i=1; i<4; ++i) {
        SDL_JoystickSetVirtualButton(joy[i], SDL_CONTROLLER_BUTTON_A+i, 1); update();
        assert(read_pad(i, 0x1000) == 0 && rd16(0x1004) == (0x1000u<<i));
    }
    /* Removing slot 1 must not renumber slots 2 and 3. */
    SDL_JoystickClose(joy[1]); assert(SDL_JoystickDetachVirtual(ids[1]) == 0); update();
    memset(G(0x1000), 0xab, 16); assert(read_pad(1, 0x1000) == 1167 && rd32(0x1000) == 0xabababab);
    assert(read_pad(2, 0x1000) == 0 && rd16(0x1004) == 0x4000);
    int replacement=SDL_JoystickAttachVirtual(SDL_JOYSTICK_TYPE_GAMECONTROLLER, SDL_CONTROLLER_AXIS_MAX, SDL_CONTROLLER_BUTTON_MAX, 0);
    assert(replacement>=0); update(); assert(read_pad(1, 0x1000) == 0);
    SDL_JoystickClose(joy[0]); SDL_JoystickClose(joy[2]); SDL_JoystickClose(joy[3]);
    SDL_Quit(); assert(read_pad(0, 0x1000) == 1167);
    free(guest_base); puts("xinput: 4 virtual pads, Xbox bits, triggers, sticks, packets, hotplug and stdcall OK");
}

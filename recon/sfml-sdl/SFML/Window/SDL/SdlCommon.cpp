#include <SFML/Window/SDL/SdlCommon.hpp>
#include <SFML/System/Err.hpp>

namespace sf
{
namespace priv
{
namespace sdl
{
namespace
{
SDL_Window*   window  = NULL;
SDL_GLContext context = NULL;

struct KeyPair { SDL_Keycode sdl; Keyboard::Key sfml; };

const KeyPair keys[] = {
    {SDLK_a, Keyboard::A}, {SDLK_b, Keyboard::B}, {SDLK_c, Keyboard::C}, {SDLK_d, Keyboard::D},
    {SDLK_e, Keyboard::E}, {SDLK_f, Keyboard::F}, {SDLK_g, Keyboard::G}, {SDLK_h, Keyboard::H},
    {SDLK_i, Keyboard::I}, {SDLK_j, Keyboard::J}, {SDLK_k, Keyboard::K}, {SDLK_l, Keyboard::L},
    {SDLK_m, Keyboard::M}, {SDLK_n, Keyboard::N}, {SDLK_o, Keyboard::O}, {SDLK_p, Keyboard::P},
    {SDLK_q, Keyboard::Q}, {SDLK_r, Keyboard::R}, {SDLK_s, Keyboard::S}, {SDLK_t, Keyboard::T},
    {SDLK_u, Keyboard::U}, {SDLK_v, Keyboard::V}, {SDLK_w, Keyboard::W}, {SDLK_x, Keyboard::X},
    {SDLK_y, Keyboard::Y}, {SDLK_z, Keyboard::Z},
    {SDLK_0, Keyboard::Num0}, {SDLK_1, Keyboard::Num1}, {SDLK_2, Keyboard::Num2}, {SDLK_3, Keyboard::Num3},
    {SDLK_4, Keyboard::Num4}, {SDLK_5, Keyboard::Num5}, {SDLK_6, Keyboard::Num6}, {SDLK_7, Keyboard::Num7},
    {SDLK_8, Keyboard::Num8}, {SDLK_9, Keyboard::Num9},
    {SDLK_ESCAPE, Keyboard::Escape}, {SDLK_LCTRL, Keyboard::LControl}, {SDLK_LSHIFT, Keyboard::LShift},
    {SDLK_LALT, Keyboard::LAlt}, {SDLK_LGUI, Keyboard::LSystem}, {SDLK_RCTRL, Keyboard::RControl},
    {SDLK_RSHIFT, Keyboard::RShift}, {SDLK_RALT, Keyboard::RAlt}, {SDLK_RGUI, Keyboard::RSystem},
    {SDLK_MENU, Keyboard::Menu}, {SDLK_LEFTBRACKET, Keyboard::LBracket}, {SDLK_RIGHTBRACKET, Keyboard::RBracket},
    {SDLK_SEMICOLON, Keyboard::Semicolon}, {SDLK_COMMA, Keyboard::Comma}, {SDLK_PERIOD, Keyboard::Period},
    {SDLK_QUOTE, Keyboard::Quote}, {SDLK_SLASH, Keyboard::Slash}, {SDLK_BACKSLASH, Keyboard::Backslash},
    {SDLK_BACKQUOTE, Keyboard::Tilde}, {SDLK_EQUALS, Keyboard::Equal}, {SDLK_MINUS, Keyboard::Hyphen},
    {SDLK_SPACE, Keyboard::Space}, {SDLK_RETURN, Keyboard::Enter}, {SDLK_BACKSPACE, Keyboard::Backspace},
    {SDLK_TAB, Keyboard::Tab}, {SDLK_PAGEUP, Keyboard::PageUp}, {SDLK_PAGEDOWN, Keyboard::PageDown},
    {SDLK_END, Keyboard::End}, {SDLK_HOME, Keyboard::Home}, {SDLK_INSERT, Keyboard::Insert},
    {SDLK_DELETE, Keyboard::Delete}, {SDLK_KP_PLUS, Keyboard::Add}, {SDLK_KP_MINUS, Keyboard::Subtract},
    {SDLK_KP_MULTIPLY, Keyboard::Multiply}, {SDLK_KP_DIVIDE, Keyboard::Divide},
    {SDLK_LEFT, Keyboard::Left}, {SDLK_RIGHT, Keyboard::Right}, {SDLK_UP, Keyboard::Up}, {SDLK_DOWN, Keyboard::Down},
    {SDLK_KP_0, Keyboard::Numpad0}, {SDLK_KP_1, Keyboard::Numpad1}, {SDLK_KP_2, Keyboard::Numpad2},
    {SDLK_KP_3, Keyboard::Numpad3}, {SDLK_KP_4, Keyboard::Numpad4}, {SDLK_KP_5, Keyboard::Numpad5},
    {SDLK_KP_6, Keyboard::Numpad6}, {SDLK_KP_7, Keyboard::Numpad7}, {SDLK_KP_8, Keyboard::Numpad8},
    {SDLK_KP_9, Keyboard::Numpad9},
    {SDLK_F1, Keyboard::F1}, {SDLK_F2, Keyboard::F2}, {SDLK_F3, Keyboard::F3}, {SDLK_F4, Keyboard::F4},
    {SDLK_F5, Keyboard::F5}, {SDLK_F6, Keyboard::F6}, {SDLK_F7, Keyboard::F7}, {SDLK_F8, Keyboard::F8},
    {SDLK_F9, Keyboard::F9}, {SDLK_F10, Keyboard::F10}, {SDLK_F11, Keyboard::F11}, {SDLK_F12, Keyboard::F12},
    {SDLK_F13, Keyboard::F13}, {SDLK_F14, Keyboard::F14}, {SDLK_F15, Keyboard::F15}, {SDLK_PAUSE, Keyboard::Pause},
};
} // namespace

void ensureInitialized()
{
    if (SDL_WasInit(SDL_INIT_VIDEO))
        return;
    SDL_SetMainReady();
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_JOYSTICK | SDL_INIT_GAMECONTROLLER) != 0)
        err() << "SDL_Init failed: " << SDL_GetError() << std::endl;
}

SDL_Window* displayWindow()
{
    if (!window)
    {
        ensureInitialized();
        window = SDL_CreateWindow("", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 960, 544,
                                  SDL_WINDOW_OPENGL | SDL_WINDOW_HIDDEN | SDL_WINDOW_RESIZABLE);
        if (!window)
            err() << "SDL_CreateWindow failed: " << SDL_GetError() << std::endl;
    }
    return window;
}

SDL_GLContext glContext()
{
    if (!context)
    {
        context = SDL_GL_CreateContext(displayWindow());
        if (!context)
            err() << "SDL_GL_CreateContext failed: " << SDL_GetError() << std::endl;
    }
    return context;
}

Keyboard::Key toSfmlKey(SDL_Keycode key)
{
    for (const KeyPair& pair : keys)
        if (pair.sdl == key)
            return pair.sfml;
    return Keyboard::Unknown;
}

SDL_Scancode toSdlScancode(Keyboard::Key key)
{
    for (const KeyPair& pair : keys)
        if (pair.sfml == key)
            return SDL_GetScancodeFromKey(pair.sdl);
    return SDL_SCANCODE_UNKNOWN;
}

} // namespace sdl
} // namespace priv
} // namespace sf

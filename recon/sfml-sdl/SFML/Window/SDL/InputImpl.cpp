#include <SFML/Window/SDL/InputImpl.hpp>
#include <SFML/Window/SDL/SdlCommon.hpp>
#include <SFML/Window/Window.hpp>

namespace sf
{
namespace priv
{
namespace
{
Uint32 buttonMask(Mouse::Button button)
{
    switch (button)
    {
        case Mouse::Left:     return SDL_BUTTON_LMASK;
        case Mouse::Right:    return SDL_BUTTON_RMASK;
        case Mouse::Middle:   return SDL_BUTTON_MMASK;
        case Mouse::XButton1: return SDL_BUTTON_X1MASK;
        case Mouse::XButton2: return SDL_BUTTON_X2MASK;
        default:              return 0;
    }
}
} // namespace

bool InputImpl::isKeyPressed(Keyboard::Key key)
{
    const SDL_Scancode code = sdl::toSdlScancode(key);
    if (code == SDL_SCANCODE_UNKNOWN)
        return false;
    int count = 0;
    const Uint8* state = SDL_GetKeyboardState(&count);
    return code < count && state[code] != 0;
}

void InputImpl::setVirtualKeyboardVisible(bool visible)
{
    if (visible) SDL_StartTextInput();
    else SDL_StopTextInput();
}

bool InputImpl::isMouseButtonPressed(Mouse::Button button)
{
    return (SDL_GetGlobalMouseState(NULL, NULL) & buttonMask(button)) != 0;
}

Vector2i InputImpl::getMousePosition()
{
    int x = 0, y = 0;
    SDL_GetGlobalMouseState(&x, &y);
    return Vector2i(x, y);
}

Vector2i InputImpl::getMousePosition(const Window&)
{
    int x = 0, y = 0;
    SDL_GetMouseState(&x, &y);
    return Vector2i(x, y);
}

void InputImpl::setMousePosition(const Vector2i& position)
{
    SDL_WarpMouseGlobal(position.x, position.y);
}

void InputImpl::setMousePosition(const Vector2i& position, const Window&)
{
    SDL_WarpMouseInWindow(sdl::displayWindow(), position.x, position.y);
}

bool InputImpl::isTouchDown(unsigned int)
{
    return false;
}

Vector2i InputImpl::getTouchPosition(unsigned int)
{
    return Vector2i();
}

Vector2i InputImpl::getTouchPosition(unsigned int, const Window&)
{
    return Vector2i();
}

} // namespace priv
} // namespace sf

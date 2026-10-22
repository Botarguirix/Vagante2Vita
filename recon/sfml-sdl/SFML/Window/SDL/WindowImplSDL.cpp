#include <SFML/Window/SDL/WindowImplSDL.hpp>
#include <SFML/Window/SDL/SdlCommon.hpp>
#include <SFML/Window/SDL/CursorImpl.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/System/Utf.hpp>
#include <string>

namespace sf
{
namespace priv
{
namespace
{
Event::KeyEvent keyEvent(const SDL_KeyboardEvent& key)
{
    Event::KeyEvent e;
    e.code    = sdl::toSfmlKey(key.keysym.sym);
    e.alt     = (key.keysym.mod & KMOD_ALT) != 0;
    e.control = (key.keysym.mod & KMOD_CTRL) != 0;
    e.shift   = (key.keysym.mod & KMOD_SHIFT) != 0;
    e.system  = (key.keysym.mod & KMOD_GUI) != 0;
    return e;
}

Mouse::Button mouseButton(Uint8 button)
{
    switch (button)
    {
        case SDL_BUTTON_LEFT:   return Mouse::Left;
        case SDL_BUTTON_RIGHT:  return Mouse::Right;
        case SDL_BUTTON_MIDDLE: return Mouse::Middle;
        case SDL_BUTTON_X1:     return Mouse::XButton1;
        default:                return Mouse::XButton2;
    }
}
} // namespace

WindowImplSDL::WindowImplSDL(WindowHandle) :
m_window   (sdl::displayWindow()),
m_keyRepeat(true)
{
}

WindowImplSDL::WindowImplSDL(VideoMode mode, const String& title, unsigned long style, const ContextSettings&) :
m_window   (sdl::displayWindow()),
m_keyRepeat(true)
{
    sdl::glContext();
    SDL_SetWindowSize(m_window, static_cast<int>(mode.width), static_cast<int>(mode.height));
    SDL_SetWindowBordered(m_window, (style & (Style::Titlebar | Style::Close)) ? SDL_TRUE : SDL_FALSE);
    SDL_SetWindowResizable(m_window, (style & Style::Resize) ? SDL_TRUE : SDL_FALSE);
    if (style & Style::Fullscreen)
        SDL_SetWindowFullscreen(m_window, SDL_WINDOW_FULLSCREEN_DESKTOP);
    SDL_SetWindowPosition(m_window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
    setTitle(title);
    SDL_ShowWindow(m_window);
#if !defined(SFML_SYSTEM_VITA)
    SDL_StartTextInput(); // on the Vita this would open the on-screen keyboard
#endif
}

WindowImplSDL::~WindowImplSDL()
{
    // The display window is shared; hide it instead of destroying it.
    SDL_HideWindow(m_window);
}

WindowHandle WindowImplSDL::getSystemHandle() const
{
    return reinterpret_cast<WindowHandle>(m_window);
}

Vector2i WindowImplSDL::getPosition() const
{
    int x = 0, y = 0;
    SDL_GetWindowPosition(m_window, &x, &y);
    return Vector2i(x, y);
}

void WindowImplSDL::setPosition(const Vector2i& position)
{
    SDL_SetWindowPosition(m_window, position.x, position.y);
}

Vector2u WindowImplSDL::getSize() const
{
    int w = 0, h = 0;
    SDL_GetWindowSize(m_window, &w, &h);
    return Vector2u(static_cast<unsigned int>(w), static_cast<unsigned int>(h));
}

void WindowImplSDL::setSize(const Vector2u& size)
{
    SDL_SetWindowSize(m_window, static_cast<int>(size.x), static_cast<int>(size.y));
}

void WindowImplSDL::setTitle(const String& title)
{
    std::string utf8;
    Utf32::toUtf8(title.begin(), title.end(), std::back_inserter(utf8));
    SDL_SetWindowTitle(m_window, utf8.c_str());
}

void WindowImplSDL::setIcon(unsigned int width, unsigned int height, const Uint8* pixels)
{
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(const_cast<Uint8*>(pixels), static_cast<int>(width),
                                                              static_cast<int>(height), 32, static_cast<int>(width * 4),
                                                              SDL_PIXELFORMAT_RGBA32);
    if (surface)
    {
        SDL_SetWindowIcon(m_window, surface);
        SDL_FreeSurface(surface);
    }
}

void WindowImplSDL::setVisible(bool visible)
{
    if (visible) SDL_ShowWindow(m_window);
    else SDL_HideWindow(m_window);
}

void WindowImplSDL::setMouseCursorVisible(bool visible)
{
    SDL_ShowCursor(visible ? SDL_ENABLE : SDL_DISABLE);
}

void WindowImplSDL::setMouseCursorGrabbed(bool grabbed)
{
    SDL_SetWindowGrab(m_window, grabbed ? SDL_TRUE : SDL_FALSE);
}

void WindowImplSDL::setMouseCursor(const CursorImpl& cursor)
{
    if (cursor.m_cursor)
        SDL_SetCursor(cursor.m_cursor);
}

void WindowImplSDL::setKeyRepeatEnabled(bool enabled)
{
    m_keyRepeat = enabled;
}

void WindowImplSDL::requestFocus()
{
    SDL_RaiseWindow(m_window);
}

bool WindowImplSDL::hasFocus() const
{
    return (SDL_GetWindowFlags(m_window) & SDL_WINDOW_INPUT_FOCUS) != 0;
}

void WindowImplSDL::processEvents()
{
    SDL_Event sdlEvent;
    while (SDL_PollEvent(&sdlEvent))
    {
        Event event;
        switch (sdlEvent.type)
        {
            case SDL_QUIT:
                event.type = Event::Closed;
                pushEvent(event);
                break;

            case SDL_WINDOWEVENT:
                switch (sdlEvent.window.event)
                {
                    case SDL_WINDOWEVENT_CLOSE:
                        event.type = Event::Closed;
                        pushEvent(event);
                        break;
                    case SDL_WINDOWEVENT_SIZE_CHANGED:
                        event.type        = Event::Resized;
                        event.size.width  = static_cast<unsigned int>(sdlEvent.window.data1);
                        event.size.height = static_cast<unsigned int>(sdlEvent.window.data2);
                        pushEvent(event);
                        break;
                    case SDL_WINDOWEVENT_FOCUS_GAINED: event.type = Event::GainedFocus; pushEvent(event); break;
                    case SDL_WINDOWEVENT_FOCUS_LOST:   event.type = Event::LostFocus;   pushEvent(event); break;
                    case SDL_WINDOWEVENT_ENTER:        event.type = Event::MouseEntered; pushEvent(event); break;
                    case SDL_WINDOWEVENT_LEAVE:        event.type = Event::MouseLeft;    pushEvent(event); break;
                    default: break;
                }
                break;

            case SDL_KEYDOWN:
            case SDL_KEYUP:
                if (sdlEvent.key.repeat && !m_keyRepeat)
                    break;
                event.type = sdlEvent.type == SDL_KEYDOWN ? Event::KeyPressed : Event::KeyReleased;
                event.key  = keyEvent(sdlEvent.key);
                pushEvent(event);
                break;

            case SDL_TEXTINPUT:
            {
                const char* begin = sdlEvent.text.text;
                const char* end   = begin + std::char_traits<char>::length(begin);
                while (begin < end)
                {
                    Uint32 codepoint = 0;
                    begin = Utf8::decode(begin, end, codepoint);
                    event.type         = Event::TextEntered;
                    event.text.unicode = codepoint;
                    pushEvent(event);
                }
                break;
            }

            case SDL_MOUSEMOTION:
                event.type        = Event::MouseMoved;
                event.mouseMove.x = sdlEvent.motion.x;
                event.mouseMove.y = sdlEvent.motion.y;
                pushEvent(event);
                break;

            case SDL_MOUSEBUTTONDOWN:
            case SDL_MOUSEBUTTONUP:
                event.type               = sdlEvent.type == SDL_MOUSEBUTTONDOWN ? Event::MouseButtonPressed
                                                                                 : Event::MouseButtonReleased;
                event.mouseButton.button = mouseButton(sdlEvent.button.button);
                event.mouseButton.x      = sdlEvent.button.x;
                event.mouseButton.y      = sdlEvent.button.y;
                pushEvent(event);
                break;

            case SDL_MOUSEWHEEL:
            {
                int x = 0, y = 0;
                SDL_GetMouseState(&x, &y);
                event.type             = Event::MouseWheelMoved;
                event.mouseWheel.delta = sdlEvent.wheel.y;
                event.mouseWheel.x     = x;
                event.mouseWheel.y     = y;
                pushEvent(event);
                event.type                   = Event::MouseWheelScrolled;
                event.mouseWheelScroll.wheel = Mouse::VerticalWheel;
                event.mouseWheelScroll.delta = static_cast<float>(sdlEvent.wheel.y);
                event.mouseWheelScroll.x     = x;
                event.mouseWheelScroll.y     = y;
                pushEvent(event);
                break;
            }

            default:
                break; // joysticks are polled through JoystickManager
        }
    }
}

} // namespace priv
} // namespace sf

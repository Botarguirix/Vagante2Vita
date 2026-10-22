// Clipboard, cursor and video modes for the SDL backend.
#include <SFML/Window/SDL/ClipboardImpl.hpp>
#include <SFML/Window/SDL/CursorImpl.hpp>
#include <SFML/Window/SDL/SdlCommon.hpp>
#include <SFML/Window/VideoModeImpl.hpp>
#include <algorithm>
#include <functional>

namespace sf
{
namespace priv
{
String ClipboardImpl::getString()
{
    sdl::ensureInitialized();
    char* text = SDL_GetClipboardText();
    if (!text)
        return String();
    String result = String::fromUtf8(text, text + std::char_traits<char>::length(text));
    SDL_free(text);
    return result;
}

void ClipboardImpl::setString(const String& text)
{
    sdl::ensureInitialized();
    const std::basic_string<Uint8> utf8 = text.toUtf8();
    SDL_SetClipboardText(reinterpret_cast<const char*>(utf8.c_str()));
}

CursorImpl::CursorImpl() :
m_cursor(NULL)
{
}

CursorImpl::~CursorImpl()
{
    release();
}

bool CursorImpl::loadFromPixels(const Uint8* pixels, Vector2u size, Vector2u hotspot)
{
    release();
    SDL_Surface* surface = SDL_CreateRGBSurfaceWithFormatFrom(const_cast<Uint8*>(pixels), static_cast<int>(size.x),
                                                              static_cast<int>(size.y), 32, static_cast<int>(size.x * 4),
                                                              SDL_PIXELFORMAT_RGBA32);
    if (!surface)
        return false;
    m_cursor = SDL_CreateColorCursor(surface, static_cast<int>(hotspot.x), static_cast<int>(hotspot.y));
    SDL_FreeSurface(surface);
    return m_cursor != NULL;
}

bool CursorImpl::loadFromSystem(Cursor::Type type)
{
    release();
    SDL_SystemCursor id = SDL_SYSTEM_CURSOR_ARROW;
    switch (type)
    {
        case Cursor::ArrowWait:          id = SDL_SYSTEM_CURSOR_WAITARROW; break;
        case Cursor::Wait:               id = SDL_SYSTEM_CURSOR_WAIT; break;
        case Cursor::Text:               id = SDL_SYSTEM_CURSOR_IBEAM; break;
        case Cursor::Hand:               id = SDL_SYSTEM_CURSOR_HAND; break;
        case Cursor::SizeHorizontal:     id = SDL_SYSTEM_CURSOR_SIZEWE; break;
        case Cursor::SizeVertical:       id = SDL_SYSTEM_CURSOR_SIZENS; break;
        case Cursor::SizeTopLeftBottomRight: id = SDL_SYSTEM_CURSOR_SIZENWSE; break;
        case Cursor::SizeBottomLeftTopRight: id = SDL_SYSTEM_CURSOR_SIZENESW; break;
        case Cursor::SizeAll:            id = SDL_SYSTEM_CURSOR_SIZEALL; break;
        case Cursor::Cross:              id = SDL_SYSTEM_CURSOR_CROSSHAIR; break;
        case Cursor::NotAllowed:         id = SDL_SYSTEM_CURSOR_NO; break;
        default: break;
    }
    m_cursor = SDL_CreateSystemCursor(id);
    return m_cursor != NULL;
}

void CursorImpl::release()
{
    if (m_cursor)
        SDL_FreeCursor(m_cursor);
    m_cursor = NULL;
}

std::vector<VideoMode> VideoModeImpl::getFullscreenModes()
{
    sdl::ensureInitialized();
    std::vector<VideoMode> modes;
    for (int i = 0; i < SDL_GetNumDisplayModes(0); ++i)
    {
        SDL_DisplayMode mode;
        if (SDL_GetDisplayMode(0, i, &mode) == 0)
        {
            const VideoMode m(static_cast<unsigned int>(mode.w), static_cast<unsigned int>(mode.h),
                              SDL_BITSPERPIXEL(mode.format) == 24 ? 32 : SDL_BITSPERPIXEL(mode.format));
            if (std::find(modes.begin(), modes.end(), m) == modes.end())
                modes.push_back(m);
        }
    }
    std::sort(modes.begin(), modes.end(), std::greater<VideoMode>());
    return modes;
}

VideoMode VideoModeImpl::getDesktopMode()
{
    sdl::ensureInitialized();
    SDL_DisplayMode mode;
    if (SDL_GetDesktopDisplayMode(0, &mode) != 0)
        return VideoMode(960, 544, 32);
    return VideoMode(static_cast<unsigned int>(mode.w), static_cast<unsigned int>(mode.h), 32);
}

} // namespace priv
} // namespace sf

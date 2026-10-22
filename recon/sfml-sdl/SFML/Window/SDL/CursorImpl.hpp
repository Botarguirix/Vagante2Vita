#ifndef SFML_CURSORIMPLSDL_HPP
#define SFML_CURSORIMPLSDL_HPP

#include <SFML/Window/Cursor.hpp>
#include <SFML/System/Vector2.hpp>
#include <SDL.h>

namespace sf
{
namespace priv
{
class CursorImpl
{
public:
    CursorImpl();
    ~CursorImpl();
    bool loadFromPixels(const Uint8* pixels, Vector2u size, Vector2u hotspot);
    bool loadFromSystem(Cursor::Type type);

private:
    friend class WindowImplSDL;
    void release();
    SDL_Cursor* m_cursor;
};

} // namespace priv
} // namespace sf

#endif

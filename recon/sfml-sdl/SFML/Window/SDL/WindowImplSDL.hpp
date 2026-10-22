#ifndef SFML_WINDOWIMPLSDL_HPP
#define SFML_WINDOWIMPLSDL_HPP

#include <SFML/Window/WindowImpl.hpp>
#include <SFML/System/String.hpp>
#include <SDL.h>

namespace sf
{
namespace priv
{
class WindowImplSDL : public WindowImpl
{
public:
    explicit WindowImplSDL(WindowHandle handle);
    WindowImplSDL(VideoMode mode, const String& title, unsigned long style, const ContextSettings& settings);
    ~WindowImplSDL();

    virtual WindowHandle getSystemHandle() const;
    virtual Vector2i getPosition() const;
    virtual void setPosition(const Vector2i& position);
    virtual Vector2u getSize() const;
    virtual void setSize(const Vector2u& size);
    virtual void setTitle(const String& title);
    virtual void setIcon(unsigned int width, unsigned int height, const Uint8* pixels);
    virtual void setVisible(bool visible);
    virtual void setMouseCursorVisible(bool visible);
    virtual void setMouseCursorGrabbed(bool grabbed);
    virtual void setMouseCursor(const CursorImpl& cursor);
    virtual void setKeyRepeatEnabled(bool enabled);
    virtual void requestFocus();
    virtual bool hasFocus() const;

protected:
    virtual void processEvents();

private:
    SDL_Window* m_window;
    bool        m_keyRepeat;
};

} // namespace priv
} // namespace sf

#endif

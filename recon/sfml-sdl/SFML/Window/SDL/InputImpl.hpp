#ifndef SFML_INPUTIMPLSDL_HPP
#define SFML_INPUTIMPLSDL_HPP

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Mouse.hpp>
#include <SFML/System/Vector2.hpp>

namespace sf
{
class Window;

namespace priv
{
class InputImpl
{
public:
    static bool isKeyPressed(Keyboard::Key key);
    static void setVirtualKeyboardVisible(bool visible);
    static bool isMouseButtonPressed(Mouse::Button button);
    static Vector2i getMousePosition();
    static Vector2i getMousePosition(const Window& relativeTo);
    static void setMousePosition(const Vector2i& position);
    static void setMousePosition(const Vector2i& position, const Window& relativeTo);
    static bool isTouchDown(unsigned int finger);
    static Vector2i getTouchPosition(unsigned int finger);
    static Vector2i getTouchPosition(unsigned int finger, const Window& relativeTo);
};

} // namespace priv
} // namespace sf

#endif

#ifndef SFML_JOYSTICKIMPLSDL_HPP
#define SFML_JOYSTICKIMPLSDL_HPP

#include <SDL.h>

namespace sf
{
namespace priv
{
// On the Vita, SDL exposes the built-in controls as joystick 0.
class JoystickImpl
{
public:
    JoystickImpl();
    static void initialize();
    static void cleanup();
    static bool isConnected(unsigned int index);
    bool open(unsigned int index);
    void close();
    JoystickCaps getCapabilities() const;
    Joystick::Identification getIdentification() const;
    JoystickState update();

private:
    SDL_Joystick*            m_joystick;
    Joystick::Identification m_identification;
};

} // namespace priv
} // namespace sf

#endif

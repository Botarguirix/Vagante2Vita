#include <SFML/Window/JoystickImpl.hpp>
#include <SFML/Window/SDL/SdlCommon.hpp>
#include <algorithm>

namespace sf
{
namespace priv
{
namespace
{
// SFML axis order X, Y, Z, R, U, V maps to SDL axes 0..5; the first hat drives PovX/PovY.
const Joystick::Axis kAxes[] = {Joystick::X, Joystick::Y, Joystick::Z, Joystick::R, Joystick::U, Joystick::V};
} // namespace

JoystickImpl::JoystickImpl() :
m_joystick(NULL)
{
}

void JoystickImpl::initialize()
{
    sdl::ensureInitialized();
}

void JoystickImpl::cleanup()
{
}

bool JoystickImpl::isConnected(unsigned int index)
{
    SDL_JoystickUpdate();
    return static_cast<int>(index) < SDL_NumJoysticks();
}

bool JoystickImpl::open(unsigned int index)
{
    m_joystick = SDL_JoystickOpen(static_cast<int>(index));
    if (!m_joystick)
        return false;
    const char* name = SDL_JoystickName(m_joystick);
    m_identification.name      = name ? String::fromUtf8(name, name + std::char_traits<char>::length(name)) : "SDL joystick";
    m_identification.vendorId  = SDL_JoystickGetVendor(m_joystick);
    m_identification.productId = SDL_JoystickGetProduct(m_joystick);
    return true;
}

void JoystickImpl::close()
{
    if (m_joystick)
        SDL_JoystickClose(m_joystick);
    m_joystick = NULL;
}

JoystickCaps JoystickImpl::getCapabilities() const
{
    JoystickCaps caps;
    if (!m_joystick)
        return caps;
    caps.buttonCount = static_cast<unsigned int>(std::min(SDL_JoystickNumButtons(m_joystick), static_cast<int>(Joystick::ButtonCount)));
    const int axes = SDL_JoystickNumAxes(m_joystick);
    for (int i = 0; i < 6; ++i)
        caps.axes[kAxes[i]] = i < axes;
    const bool hat = SDL_JoystickNumHats(m_joystick) > 0;
    caps.axes[Joystick::PovX] = hat;
    caps.axes[Joystick::PovY] = hat;
    return caps;
}

Joystick::Identification JoystickImpl::getIdentification() const
{
    return m_identification;
}

JoystickState JoystickImpl::update()
{
    JoystickState state;
    if (!m_joystick || !SDL_JoystickGetAttached(m_joystick))
        return state;
    state.connected = true;
    const int axes = std::min(SDL_JoystickNumAxes(m_joystick), 6);
    for (int i = 0; i < axes; ++i)
        state.axes[kAxes[i]] = SDL_JoystickGetAxis(m_joystick, i) * 100.f / 32767.f;
    const int buttons = std::min(SDL_JoystickNumButtons(m_joystick), static_cast<int>(Joystick::ButtonCount));
    for (int i = 0; i < buttons; ++i)
        state.buttons[i] = SDL_JoystickGetButton(m_joystick, i) != 0;
    if (SDL_JoystickNumHats(m_joystick) > 0)
    {
        const Uint8 hat = SDL_JoystickGetHat(m_joystick, 0);
        state.axes[Joystick::PovX] = (hat & SDL_HAT_RIGHT) ? 100.f : (hat & SDL_HAT_LEFT) ? -100.f : 0.f;
        state.axes[Joystick::PovY] = (hat & SDL_HAT_UP) ? 100.f : (hat & SDL_HAT_DOWN) ? -100.f : 0.f;
    }
    return state;
}

} // namespace priv
} // namespace sf

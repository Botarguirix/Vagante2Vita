// SDL2 platform backend for SFML 2.5 (PC test builds and PS Vita through SDL2 + vitaGL).
// Vagante has a single window, and vitaGL a single GL context, so every sf::Window and every
// sf::Context share one SDL window ("the display") and one SDL_GLContext.
#ifndef SFML_SDLCOMMON_HPP
#define SFML_SDLCOMMON_HPP

#include <SFML/Window/Keyboard.hpp>
#define SDL_MAIN_HANDLED  // the game keeps its own main(); SDL_SetMainReady() is called instead
#include <SDL.h>

namespace sf
{
namespace priv
{
namespace sdl
{
// Initializes SDL video/joystick/game controller once.
void ensureInitialized();

// The shared display window, created hidden on first use.
SDL_Window* displayWindow();

// The shared GL context, created on first use (makes it current).
SDL_GLContext glContext();

// Key translation between SDL and SFML.
Keyboard::Key toSfmlKey(SDL_Keycode key);
SDL_Scancode toSdlScancode(Keyboard::Key key);
} // namespace sdl
} // namespace priv
} // namespace sf

#endif

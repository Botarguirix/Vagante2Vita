#include <SFML/Window/SDL/SdlContext.hpp>
#include <SFML/Window/SDL/SdlCommon.hpp>

namespace sf
{
namespace priv
{
SdlContext::SdlContext(SdlContext*)
{
    sdl::glContext();
}

SdlContext::SdlContext(SdlContext*, const ContextSettings& settings, const WindowImpl*, unsigned int)
{
    m_settings = settings;
    sdl::glContext();
}

SdlContext::SdlContext(SdlContext*, const ContextSettings& settings, unsigned int, unsigned int)
{
    m_settings = settings;
    sdl::glContext();
}

SdlContext::~SdlContext()
{
    // The shared SDL_GLContext lives until the process exits.
}

GlFunctionPointer SdlContext::getFunction(const char* name)
{
    return reinterpret_cast<GlFunctionPointer>(SDL_GL_GetProcAddress(name));
}

bool SdlContext::makeCurrent(bool current)
{
    if (!current)
        return true; // a single context: deactivation is a no-op
    return SDL_GL_MakeCurrent(sdl::displayWindow(), sdl::glContext()) == 0;
}

void SdlContext::display()
{
    SDL_GL_SwapWindow(sdl::displayWindow());
}

void SdlContext::setVerticalSyncEnabled(bool enabled)
{
    SDL_GL_SetSwapInterval(enabled ? 1 : 0);
}

} // namespace priv
} // namespace sf

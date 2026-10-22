#ifndef SFML_SDLCONTEXT_HPP
#define SFML_SDLCONTEXT_HPP

#include <SFML/Window/GlContext.hpp>

namespace sf
{
namespace priv
{
class WindowImpl;

// Every SdlContext is a handle on the one shared SDL_GLContext (see SdlCommon.hpp).
class SdlContext : public GlContext
{
public:
    explicit SdlContext(SdlContext* shared);
    SdlContext(SdlContext* shared, const ContextSettings& settings, const WindowImpl* owner, unsigned int bitsPerPixel);
    SdlContext(SdlContext* shared, const ContextSettings& settings, unsigned int width, unsigned int height);
    ~SdlContext();

    static GlFunctionPointer getFunction(const char* name);

    virtual void display();
    virtual void setVerticalSyncEnabled(bool enabled);

protected:
    virtual bool makeCurrent(bool current);
};

} // namespace priv
} // namespace sf

#endif

// OpenGL queries kept out of the game headers: <SFML/OpenGL.hpp> pulls <windows.h> on Windows,
// which clashes with the Win32 placeholder types the reconstructed headers declare.
#include <SFML/OpenGL.hpp>

namespace recon {
const char *glVendor() { return reinterpret_cast<const char *>(glGetString(GL_VENDOR)); }
const char *glRenderer() { return reinterpret_cast<const char *>(glGetString(GL_RENDERER)); }
}

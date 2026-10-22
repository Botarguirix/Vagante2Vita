#ifndef SFML_CLIPBOARDIMPLSDL_HPP
#define SFML_CLIPBOARDIMPLSDL_HPP

#include <SFML/System/String.hpp>

namespace sf
{
namespace priv
{
class ClipboardImpl
{
public:
    static String getString();
    static void setString(const String& text);
};

} // namespace priv
} // namespace sf

#endif

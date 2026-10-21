// Support for generated stubs of functions that are not translated yet.
#pragma once
#include <cstddef>
#include <cstdlib>
#include <type_traits>

namespace recon {
// Logs the first call of each untranslated function (stderr and recon.log).
void unimplemented(const char *function);

// A placeholder value for a stub's return type.
template<class T> T stub_value() {
    if constexpr (std::is_void_v<T>) {
        return;
    } else if constexpr (std::is_reference_v<T>) {
        // Zeroed storage large enough for any game object (Player is ~31 KB on x86).
        alignas(std::max_align_t) static unsigned char storage[1 << 16] = {};
        return static_cast<T>(*reinterpret_cast<std::remove_reference_t<T> *>(storage));
    } else if constexpr (std::is_default_constructible_v<T>) {
        return T{};
    } else {
        std::abort();
    }
}
}

#define RECON_UNIMPLEMENTED(name) ::recon::unimplemented(name)

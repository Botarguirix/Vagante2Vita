// Host-compiler check for uint32 -> binary32 rounding used by the recovered RNG.
#include <cstdint>
#include <cstring>
#include <iostream>
#include <random>
#include <string>

int main(int argc, char **argv) {
    if (argc != 2) return 2;
    const auto seed = std::stoull(argv[1]);
    if (seed > UINT32_MAX) return 2;
    std::mt19937 engine(static_cast<uint32_t>(seed));
    for (int i = 0; i < 1000; ++i) {
        const uint32_t word = engine();
        volatile float converted = static_cast<float>(word);
        const float value = converted / 4294967296.0f;
        uint32_t bits;
        static_assert(sizeof(value) == sizeof(bits));
        std::memcpy(&bits, &value, sizeof(bits));
        std::cout << word << ' ' << bits << '\n';
    }
    return 0;
}

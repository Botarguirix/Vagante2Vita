#ifndef SFML_SENSORIMPLSDL_HPP
#define SFML_SENSORIMPLSDL_HPP

#include <SFML/Window/Sensor.hpp>

namespace sf
{
namespace priv
{
// Vagante does not use motion sensors.
class SensorImpl
{
public:
    static void initialize() {}
    static void cleanup() {}
    static bool isAvailable(Sensor::Type) { return false; }
    bool open(Sensor::Type) { return false; }
    void close() {}
    Vector3f update() { return Vector3f(); }
    void setEnabled(bool) {}
};

} // namespace priv
} // namespace sf

#endif

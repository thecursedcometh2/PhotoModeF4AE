#pragma once
#include <cstdint>

namespace PhotoMode {
// Track Photo Mode movement only. Leave later changes by other mods alone.
class VerticalInputOwner {
public:
    void Record(const void* state, std::int16_t direction)
    {
        owner = direction != 0 ? state : nullptr;
        lastDirection = direction;
    }
    void Release(const void* currentState, std::int16_t& direction)
    {
        if (owner && owner == currentState && direction == lastDirection) direction = 0;
        owner = nullptr;
        lastDirection = 0;
    }
private:
    const void* owner{};
    std::int16_t lastDirection{};
};
}

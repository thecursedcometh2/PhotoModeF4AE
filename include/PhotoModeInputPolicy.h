#pragma once
#include <cstdint>

namespace PhotoMode {
// Track only movement synthesized by Photo Mode. Never reset another camera
// state or a direction subsequently changed by another mod.
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

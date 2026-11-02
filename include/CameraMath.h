#pragma once
#include <cmath>

namespace CameraMath
{
    struct Quaternion { float w, x, y, z; };
    // Camera forward is local Y.
    inline Quaternion ApplyLocalRoll(Quaternion q, float radians)
    {
        if (!std::isfinite(radians)) return q;
        const float c = std::cos(radians * 0.5f);
        const float s = std::sin(radians * 0.5f);
        return {q.w * c - q.y * s, q.x * c - q.z * s,
            q.y * c + q.w * s, q.z * c + q.x * s};
    }
}

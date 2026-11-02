#pragma once
#include <array>
#include <cmath>

namespace CameraMath
{
    using Rotation = std::array<std::array<float, 3>, 3>;

    inline Rotation RollRotation(Rotation base, float radians)
    {
        if (!std::isfinite(radians)) return base;
        const float c = std::cos(radians);
        const float s = std::sin(radians);
        // Post-multiply around local Y (camera forward); preserve its direction.
        for (auto& row : base) {
            const float x = row[0];
            const float z = row[2];
            row[0] = x * c - z * s;
            row[2] = x * s + z * c;
        }
        return base;
    }

    inline bool SameRotation(const Rotation& a, const Rotation& b)
    {
        for (std::size_t row = 0; row < 3; ++row) {
            for (std::size_t col = 0; col < 3; ++col) {
                if (!std::isfinite(a[row][col]) || !std::isfinite(b[row][col]) ||
                    std::abs(a[row][col] - b[row][col]) > 1e-5f) return false;
            }
        }
        return true;
    }

    // Restore only our last output on the same node. Never undo a newer writer.
    // Restoring before every camera update prevents frame-to-frame accumulation,
    // even if the original updater leaves the rotation unchanged while frozen.
    class RollOverlay
    {
    public:
        Rotation Apply(const void* node, const Rotation& rotation, float radians)
        {
            owner = node;
            base = rotation;
            applied = RollRotation(base, radians);
            return applied;
        }
        bool Release(const void* node, Rotation& rotation)
        {
            const bool restore = owner && owner == node && SameRotation(rotation, applied);
            if (restore) rotation = base;
            owner = nullptr;
            return restore;
        }
    private:
        const void* owner{};
        Rotation base{}, applied{};
    };
}

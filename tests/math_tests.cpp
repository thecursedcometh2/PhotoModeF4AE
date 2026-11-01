#include "ImageMath.h"
#include "CameraMath.h"
#include <array>
#include <cassert>
#include <limits>

int main()
{
    // Three rows with padding exercises complete row coverage and preserves padding.
    std::array<std::uint8_t, 36> base{}, overlay{};
    base.fill(17);
    overlay.fill(0);
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 2; ++x) {
            const int i = y * 12 + x * 4;
            base[i] = 100; base[i+1] = 50; base[i+2] = 0;
            overlay[i] = 200; overlay[i+1] = 100; overlay[i+2] = 80; overlay[i+3] = 255;
        }
    }
    assert(ImageMath::CompositeOpaque(base.data(), 12, overlay.data(), 12, 2, 3, 0.5f));
    for (int y = 0; y < 3; ++y) {
        for (int x = 0; x < 2; ++x) {
            const int i = y * 12 + x * 4;
            assert(base[i] == 150 && base[i+1] == 75 && base[i+2] == 40 && base[i+3] == 255);
        }
        assert(base[y*12+8] == 17);
    }
    overlay.fill(0);
    base[3] = 0;
    assert(ImageMath::CompositeOpaque(base.data(), 12, overlay.data(), 12, 2, 3, 1));
    assert(base[0] == 150 && base[3] == 255);
    assert(!ImageMath::CompositeOpaque(base.data(), 1, overlay.data(), 12, 2, 3, 1));
    assert(!ImageMath::CompositeOpaque(nullptr, 12, overlay.data(), 12, 2, 3, 1));
    assert(!ImageMath::CompositeOpaque(base.data(), 12, overlay.data(), 12, 2, 3, std::numeric_limits<float>::quiet_NaN()));
    const CameraMath::Quaternion q{0.5f, 0.5f, 0.5f, 0.5f};
    const auto unchanged = CameraMath::ApplyLocalRoll(q, 0);
    assert(unchanged.w == q.w && unchanged.x == q.x && unchanged.y == q.y && unchanged.z == q.z);
    const auto invalid = CameraMath::ApplyLocalRoll(q, std::numeric_limits<float>::quiet_NaN());
    assert(invalid.w == q.w && invalid.x == q.x && invalid.y == q.y && invalid.z == q.z);
    const auto rolled = CameraMath::ApplyLocalRoll(q, 1.2f);
    const auto restored = CameraMath::ApplyLocalRoll(rolled, -1.2f);
    assert(std::abs(restored.w-q.w) < 1e-6f && std::abs(restored.x-q.x) < 1e-6f &&
        std::abs(restored.y-q.y) < 1e-6f && std::abs(restored.z-q.z) < 1e-6f);
}

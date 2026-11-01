#pragma once
#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <limits>

namespace ImageMath
{
    // Both buffers are straight-alpha RGBA8 in the same color space.
    // The game capture is a flattened opaque background, not an alpha layer.
    inline bool CompositeOpaque(std::uint8_t* base, std::size_t basePitch,
        const std::uint8_t* overlay, std::size_t overlayPitch,
        std::size_t width, std::size_t height, float intensity)
    {
        if (!base || !overlay || !width || !height || !std::isfinite(intensity) ||
            width > std::numeric_limits<std::size_t>::max() / 4 ||
            basePitch < width * 4 || overlayPitch < width * 4 ||
            height > std::numeric_limits<std::size_t>::max() / basePitch ||
            height > std::numeric_limits<std::size_t>::max() / overlayPitch) {
            return false;
        }
        intensity = std::clamp(intensity, 0.0f, 1.0f);
        // Sequential processing avoids per-shot thread creation, zero-concurrency
        // division, lost remainder rows, and exceptions while threads are active.
        for (std::size_t y = 0; y < height; ++y) {
            auto* dst = base + y * basePitch;
            const auto* src = overlay + y * overlayPitch;
            for (std::size_t x = 0; x < width; ++x) {
                const auto offset = x * 4;
                const float alpha = src[offset + 3] / 255.0f * intensity;
                for (std::size_t channel = 0; channel < 3; ++channel) {
                    dst[offset + channel] = static_cast<std::uint8_t>(std::lround(
                        src[offset + channel] * alpha + dst[offset + channel] * (1.0f - alpha)));
                }
                dst[offset + 3] = 255;
            }
        }
        return true;
    }
}

#include "Graphics.h"
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool passed, const char* message)
    {
        if (!passed) throw std::runtime_error(message);
    }
}

int main()
{
    const auto initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized)) return 1;
    const auto path = std::filesystem::temp_directory_path() /
        std::format("photomode-image-test-{}.png", GetCurrentProcessId());
    int result = 0;
    try {
        // Distinct BGRA channels and zero capture alpha catch both transfer and
        // channel regressions. No game or renderer objects are touched here.
        DirectX::ScratchImage bgra;
        require(SUCCEEDED(bgra.Initialize2D(DXGI_FORMAT_B8G8R8A8_UNORM_SRGB, 2, 3, 1, 1)), "initialize BGRA");
        auto* image = bgra.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < 3; ++y) {
            for (std::size_t x = 0; x < 2; ++x) {
                auto* pixel = image->pixels + y * image->rowPitch + x * 4;
                pixel[0] = 10; pixel[1] = 20; pixel[2] = static_cast<std::uint8_t>(90 + y); pixel[3] = 0;
            }
        }
        require(Texture::SaveToPNG(bgra, path.string()), "save opaque PNG from sRGB BGRA");
        DirectX::ScratchImage decoded, rgba;
        require(SUCCEEDED(DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, decoded)), "decode PNG");
        require(SUCCEEDED(Texture::ToRGBA8(*decoded.GetImage(0, 0, 0), rgba)), "normalize decoded PNG");
        image = rgba.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < 3; ++y) {
            for (std::size_t x = 0; x < 2; ++x) {
                const auto* pixel = image->pixels + y * image->rowPitch + x * 4;
                require(pixel[0] == 90 + y && pixel[1] == 20 && pixel[2] == 10, "PNG RGB channels/transfer changed");
                require(pixel[3] == 255, "PNG capture alpha is not opaque");
            }
        }
        DirectX::ScratchImage overlay, blended;
        require(SUCCEEDED(overlay.Initialize2D(DXGI_FORMAT_R8G8B8A8_UNORM, 2, 3, 1, 1)), "initialize overlay");
        image = overlay.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < 3; ++y) {
            for (std::size_t x = 0; x < 2; ++x) {
                auto* pixel = image->pixels + y * image->rowPitch + x * 4;
                pixel[0] = 200; pixel[1] = 100; pixel[2] = 80; pixel[3] = x == 0 ? 128 : 0;
            }
        }
        require(Texture::AlphaBlendImage(rgba.GetImage(0, 0, 0), overlay.GetImage(0, 0, 0), blended, 0.5f), "blend overlay");
        require(Texture::SaveToPNG(blended, path.string()), "save overlay PNG");
        decoded.Release();
        require(SUCCEEDED(DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, decoded)), "decode overlay PNG");
        rgba.Release();
        require(SUCCEEDED(Texture::ToRGBA8(*decoded.GetImage(0, 0, 0), rgba)), "normalize overlay PNG");
        image = rgba.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < 3; ++y) {
            const auto* row = image->pixels + y * image->rowPitch;
            const float alpha = 128.0f / 255.0f * 0.5f;
            require(row[0] == std::lround(200 * alpha + (90 + y) * (1 - alpha)), "overlay blend RGB");
            require(row[3] == 255 && row[7] == 255, "overlay PNG alpha");
            require(row[4] == 90 + y && row[5] == 20 && row[6] == 10, "transparent overlay changed RGB");
        }
        const auto invalid = path / "not-a-directory" / "image.png";
        require(!Texture::SaveToPNG(blended, invalid.string()), "encoding error was not returned");
        std::cout << "Windows image checks passed: BGRA/sRGB channels, opaque PNG roundtrip, overlays, all rows, encode failure\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        result = 1;
    }
    std::error_code ignored;
    std::filesystem::remove(path, ignored);
    CoUninitialize();
    return result;
}

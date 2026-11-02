#include "Graphics.h"
#include "ScreenshotFiles.h"
#include <fstream>
#include <barrier>
#include "OverlayImage.h"
#include <thread>
#include "FontAssetPath.h"
#include "WidgetID.h"
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
    const auto photos = path.parent_path() / path.stem();
    int result = 0;
    try {
        require(Font::AssetPath("Roboto-Regular.ttf").generic_string() == "Data/Interface/ImGuiFonts/Roboto-Regular.ttf", "default font without fonts.ini");
        require(Font::AssetPath("Data/Interface/ImGuiFonts/Roboto-Regular.ttf").generic_string() == "Data/Interface/ImGuiFonts/Roboto-Regular.ttf", "font prefix duplicated");
        Translation::Manager translations;
        require(translations.GetTranslation("$PM_ViewRoll") == "View Roll", "English defaults without translation file");
        const auto* missing = translations.GetTranslation("$MissingOne").c_str();
        for (int i = 0; i < 100; ++i) translations.GetTranslation(std::format("$Missing{}", i));
        require(std::string(missing) == "$MissingOne", "missing-key label lifetime");
        ImGui::CreateContext();
        ImGui::GetIO().DisplaySize = ImVec2(800, 600);
        ImGui::GetIO().DeltaTime = 1.0f / 60.0f;
        ImGui::GetIO().Fonts->AddFontDefault();
        ImGui::GetIO().Fonts->Build();
        ImGui::NewFrame();
        ImGui::Begin("ID regression");
        float first = 0, second = 0;
        ImGuiID firstID, secondID;
        { ImGui::ScopedWidgetID scope(&first); firstID = ImGui::GetID("Identical translation"); }
        { ImGui::ScopedWidgetID scope(&second); secondID = ImGui::GetID("Identical translation"); }
        require(firstID != secondID, "translated sliders share an ImGui ID");
        ImGui::End(); ImGui::EndFrame(); ImGui::DestroyContext();
        std::cout << "UI checks passed: English defaults, font paths without INI, stable fallback labels, isolated widget IDs\n";
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
        HRESULT workerLoad = E_FAIL;
        std::thread worker([&] {
            DirectX::ScratchImage overlayImage;
            workerLoad = PhotoMode::LoadOverlayImage(path, overlayImage);
        });
        worker.join();
        require(SUCCEEDED(workerLoad), "overlay decoding on a thread without a pre-existing COM apartment");
        std::filesystem::create_directory(photos);
        const auto prefix = photos / "Screenshot";
        const auto readBytes = [](const std::filesystem::path& file) {
            std::ifstream stream(file, std::ios::binary);
            return std::string(std::istreambuf_iterator<char>(stream), {});
        };
        std::uint64_t next = 0;
        std::filesystem::path saved;
        require(Texture::SaveNewPNG(bgra, prefix, next, saved), "first session photo 0");
        DirectX::ScratchImage stored, storedRGBA;
        require(SUCCEEDED(PhotoMode::LoadOverlayImage(saved, stored)), "decode exclusively created PNG");
        require(SUCCEEDED(Texture::ToRGBA8(*stored.GetImage(0, 0, 0), storedRGBA)), "normalize exclusively created PNG");
        const auto* storedImage = storedRGBA.GetImage(0, 0, 0);
        require(storedImage->width == 2 && storedImage->height == 3, "new PNG dimensions");
        for (std::size_t y = 0; y < 3; ++y) {
            for (std::size_t x = 0; x < 2; ++x) {
                const auto* pixel = storedImage->pixels + y * storedImage->rowPitch + x * 4;
                require(pixel[0] == 90 + y && pixel[1] == 20 && pixel[2] == 10 && pixel[3] == 255, "exclusive PNG colors/opacity");
            }
        }
        const auto firstBytes = readBytes(saved);
        ++next;
        require(Texture::SaveNewPNG(bgra, prefix, next, saved), "first session photo 1");
        const auto secondBytes = readBytes(saved);
        // New photos deliberately contain different pixels, so byte comparisons
        // detect replacement rather than merely rewriting an identical PNG.
        const auto* newScene = bgra.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < newScene->height; ++y) {
            for (std::size_t x = 0; x < newScene->width; ++x) {
                newScene->pixels[y * newScene->rowPitch + x * 4 + 2] = static_cast<std::uint8_t>(180 + y);
            }
        }
        // Simulate a restart with no persisted index/configuration.
        std::error_code scanError;
        next = Screenshot::NextFileIndex(prefix, 0, scanError);
        require(!scanError && next == 2, "restart directory scan without INI");
        require(Texture::SaveNewPNG(bgra, prefix, next, saved), "second session photo 2");
        ++next;
        require(Texture::SaveNewPNG(bgra, prefix, next, saved), "second session photo 3");
        require(readBytes(photos / "Screenshot0.png") == firstBytes && readBytes(photos / "Screenshot1.png") == secondBytes, "restart overwrote photos");
        // Force a stale index after a successful scan: CREATE_NEW must still skip.
        next = 0;
        require(Texture::SaveNewPNG(bgra, prefix, next, saved) && next == 4, "exclusive collision retry");
        require(readBytes(photos / "Screenshot0.png") == firstBytes, "collision overwrote photo");
        std::ofstream(photos / "Screenshot4294967295.png", std::ios::binary) << "legacy photo";
        next = Screenshot::NextFileIndex(prefix, 0, scanError);
        require(next == 4294967296ULL, "legacy unsigned -1 filename scan");
        require(Texture::SaveNewPNG(bgra, prefix, next, saved), "64-bit screenshot save");
        require(readBytes(photos / "Screenshot4294967295.png") == "legacy photo", "legacy photo overwritten");
        std::array<bool, 2> writers{};
        std::array<std::filesystem::path, 2> writerPaths;
        std::barrier gate(2);
        const auto writeConcurrent = [&](std::size_t slot) {
            const auto com = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
            std::uint64_t candidate = 0;
            gate.arrive_and_wait();
            if (SUCCEEDED(com)) {
                writers[slot] = Texture::SaveNewPNG(bgra, photos / "Concurrent", candidate, writerPaths[slot]);
                CoUninitialize();
            }
        };
        std::thread writerA(writeConcurrent, 0), writerB(writeConcurrent, 1);
        writerA.join(); writerB.join();
        require(writers[0] && writers[1] && writerPaths[0] != writerPaths[1], "concurrent writers reused a filename");
        std::ofstream(photos / "Max18446744073709551615.png", std::ios::binary) << "last photo";
        next = UINT64_MAX;
        require(!Texture::SaveNewPNG(bgra, photos / "Max", next, saved), "exhaustion wrapped around");
        require(readBytes(photos / "Max18446744073709551615.png") == "last photo", "exhaustion overwrote photo");
        std::cout << "Screenshot preservation checks passed: restart without INI, stale index, legacy 4294967295, 64-bit filenames, concurrent writers, exhaustion\n";
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
    std::filesystem::remove_all(photos, ignored);
    CoUninitialize();
    return result;
}

#pragma once
#include <Windows.h>
#include <DirectXTex.h>
#include <filesystem>

namespace PhotoMode {
inline HRESULT LoadOverlayImage(const std::filesystem::path& path, DirectX::ScratchImage& image)
{
    // GameDataReady may run on a thread that has not initialized COM. Existing
    // STA/MTA apartments are retained; only balance our successful initialization.
    const HRESULT initialized = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(initialized) && initialized != RPC_E_CHANGED_MODE) return initialized;
    const HRESULT loaded = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, image);
    if (SUCCEEDED(initialized)) CoUninitialize();
    return loaded;
}
}

#pragma once
#include <filesystem>
namespace Font {
inline std::filesystem::path AssetPath(const std::filesystem::path& configured)
{
    const auto path = configured.lexically_normal();
    const auto generic = path.generic_string();
    if (path.is_absolute() || generic.starts_with("Data/") || generic.starts_with("data/")) return path;
    return std::filesystem::path("Data/Interface/ImGuiFonts") / path;
}
}

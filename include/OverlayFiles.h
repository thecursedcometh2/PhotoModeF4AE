#pragma once
#include <algorithm>
#include <cctype>
#include <filesystem>
#include <map>
#include <string>
#include <vector>

namespace PhotoMode {
using OverlayFiles = std::map<std::string, std::vector<std::filesystem::path>>;
inline OverlayFiles FindOverlayFiles(const std::filesystem::path& root, std::error_code& error)
{
    OverlayFiles files;
    std::filesystem::recursive_directory_iterator it(root, error), end;
    while (!error && it != end) {
        const auto& entry = *it;
        if (entry.is_regular_file(error)) {
            auto extension = entry.path().extension().string();
            std::transform(extension.begin(), extension.end(), extension.begin(),
                [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
            if (extension == ".png") {
                const auto category = entry.path().parent_path().lexically_relative(root).generic_string();
                files[category == "." ? "General" : category].push_back(entry.path());
            }
        }
        it.increment(error);
    }
    for (auto& [category, paths] : files) std::sort(paths.begin(), paths.end());
    return files;
}
}

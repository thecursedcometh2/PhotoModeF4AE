#pragma once
#include <algorithm>
#include <charconv>
#include <cstdint>
#include <filesystem>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace Screenshot {
inline std::optional<std::uint64_t> ParseIndex(std::string_view text)
{
    std::uint64_t value{};
    const auto [end, error] = std::from_chars(text.data(), text.data() + text.size(), value);
    if (error != std::errc{} || end != text.data() + text.size() || text.empty()) return std::nullopt;
    return value;
}
inline std::string FoldFilename(const std::filesystem::path& path)
{
    const auto utf8 = path.u8string();
    std::string text(utf8.begin(), utf8.end());
    for (auto& ch : text) if (ch >= 'A' && ch <= 'Z') ch += 'a' - 'A';
    return text;
}
// Prefix contains the directory and basename, including any configured separator.
inline std::uint64_t NextFileIndex(const std::filesystem::path& prefix, std::uint64_t hint, std::error_code& error)
{
    auto directory = prefix.parent_path();
    if (directory.empty()) directory = ".";
    const auto name = FoldFilename(prefix.filename());
    auto next = hint;
    std::filesystem::directory_iterator it(directory, error), end;
    while (!error && it != end) {
        if (it->is_regular_file(error)) {
            const auto filename = FoldFilename(it->path().filename());
            if (filename.starts_with(name) && filename.ends_with(".png")) {
                const auto digits = std::string_view(filename).substr(name.size(), filename.size() - name.size() - 4);
                if (const auto number = ParseIndex(digits)) {
                    const auto successor = *number == std::numeric_limits<std::uint64_t>::max() ? *number : *number + 1;
                    next = std::max(next, successor);
                }
            }
        }
        if (!error) it.increment(error);
    }
    return next;
}
inline std::filesystem::path NumberedPath(const std::filesystem::path& prefix, std::uint64_t number)
{
    auto path = prefix;
    path += std::to_string(number) + ".png";
    return path;
}
}

#pragma once
#include <charconv>
#include <cstdint>
#include <optional>
#include <string_view>

namespace PhotoMode {
// f4mcm keyinput/ModSettingString stores "virtual-key,modifier-mask".
// In-mode bindings are single keys (allowModifierKeys=0); legacy i* keys remain supported.
inline std::optional<std::uint32_t> ParseMCMKey(std::string_view text, bool gamepad)
{
    const auto comma = text.find(',');
    const auto keyText = text.substr(0, comma);
    if (keyText.empty()) return std::nullopt;
    if (comma != text.npos && text.substr(comma + 1) != "0") return std::nullopt;
    std::uint32_t key{};
    const auto [end, error] = std::from_chars(keyText.data(), keyText.data() + keyText.size(), key);
    if (error != std::errc{} || end != keyText.data() + keyText.size()) return std::nullopt;
    if (key == 0 || (gamepad ? key >= 266 && key <= 281 : key > 0 && key < 266)) return key;
    return std::nullopt;
}

// MCM dropdown indices: 0 = unbound; 1..16 = F4SE gamepad codes 266..281.
inline std::optional<std::uint32_t> ControllerBinding(long selection)
{
    if (selection < 0 || selection > 16) return std::nullopt;
    return selection == 0 ? 0U : static_cast<std::uint32_t>(265 + selection);
}
}

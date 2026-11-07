#pragma once
#include <string>
#include <string_view>

namespace Translation
{
    template <class Lookup>
    std::string ResolveGameLanguage(Lookup lookup)
    {
        const auto* setting = lookup("sLanguage:General");
        const auto value = setting ? setting->GetString() : std::string_view{};
        if (value.empty()) return "EN";
        std::string language(value);
        for (auto& c : language) if (c >= 'a' && c <= 'z') c = static_cast<char>(c - 'a' + 'A');
        return language;
    }
}

#pragma once

#include <Windows.h>
#include <ShlObj.h>
#include <spdlog/spdlog.h>

// Small project-owned adapters for utilities removed from the old CommonLib API.
namespace logger
{
    template <class... Args>
    void info(std::string_view format, Args&&... args)
    {
        spdlog::info("{}", std::vformat(format, std::make_format_args(args...)));
    }
    template <class... Args>
    void error(std::string_view format, Args&&... args)
    {
        spdlog::error("{}", std::vformat(format, std::make_format_args(args...)));
    }
    inline std::optional<std::filesystem::path> log_directory()
    {
        PWSTR documents = nullptr;
        if (FAILED(SHGetKnownFolderPath(FOLDERID_Documents, 0, nullptr, &documents))) {
            return std::nullopt;
        }
        auto path = std::filesystem::path(documents) / "My Games" / F4SE::GetSaveFolderName() / "F4SE";
        CoTaskMemFree(documents);
        return path;
    }
}

namespace stl
{
    [[noreturn]] inline void report_and_fail(std::string_view message)
    {
        logger::error("{}", message);
        MessageBoxA(nullptr, std::string(message).c_str(), "Photo Mode", MB_OK | MB_ICONERROR);
        std::terminate();
    }
    inline std::optional<std::wstring> utf8_to_utf16(std::string_view text)
    {
        if (text.empty()) return std::wstring{};
        const auto size = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0);
        if (size == 0) return std::nullopt;
        std::wstring result(size, L'\0');
        if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size)) return std::nullopt;
        return result;
    }
    inline std::optional<std::string> utf16_to_utf8(std::wstring_view text)
    {
        if (text.empty()) return std::string{};
        const auto size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
        if (size == 0) return std::nullopt;
        std::string result(size, '\0');
        if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()), result.data(), size, nullptr, nullptr)) return std::nullopt;
        return result;
    }
}

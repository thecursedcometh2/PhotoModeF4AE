#pragma once
#include <windows.h>
#include <algorithm>
#include <array>
#include <cstdint>
#include <format>
#include <limits>
#include <optional>
#include <string>
#include <string_view>

namespace CameraTrace
{
    inline std::optional<std::uintptr_t> ReadPointer(std::uintptr_t address)
    {
        std::uintptr_t result{};
        SIZE_T read{};
        if (!address || !ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
                &result, sizeof(result), &read) || read != sizeof(result)) return std::nullopt;
        return result;
    }

    inline std::optional<std::uintptr_t> VtableEntry(std::uintptr_t object, std::size_t slot)
    {
        const auto table = ReadPointer(object);
        if (!table || slot > ((std::numeric_limits<std::uintptr_t>::max)() - *table) / sizeof(void*))
            return std::nullopt;
        return ReadPointer(*table + slot * sizeof(void*));
    }

    // Log image basename + RVA, never a personal installation path.
    inline std::string Describe(std::uintptr_t address)
    {
        if (!address) return "null";
        HMODULE module{};
        if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                reinterpret_cast<LPCWSTR>(address), &module)) return "outside loaded images";
        std::array<char, 1024> path{};
        const auto length = GetModuleFileNameA(module, path.data(), static_cast<DWORD>(path.size()));
        std::string_view name = "image";
        if (length > 0 && length < path.size()) {
            name = std::string_view(path.data(), length);
            name.remove_prefix(name.find_last_of("/\\") == name.npos ? 0 : name.find_last_of("/\\") + 1);
        }
        return std::format("{}+{:X}", name, address - reinterpret_cast<std::uintptr_t>(module));
    }

    // At most 192 bytes, only from executable pages in the main EXE image.
    // This is a read-only code snapshot, not a prologue size or patch offset.
    inline std::string CodeBytes(std::uintptr_t address)
    {
        MEMORY_BASIC_INFORMATION region{};
        constexpr DWORD executable = PAGE_EXECUTE | PAGE_EXECUTE_READ | PAGE_EXECUTE_READWRITE | PAGE_EXECUTE_WRITECOPY;
        if (!VirtualQuery(reinterpret_cast<const void*>(address), &region, sizeof(region)) ||
            region.State != MEM_COMMIT || region.Type != MEM_IMAGE ||
            region.AllocationBase != GetModuleHandleW(nullptr) || !(region.Protect & executable) ||
            (region.Protect & (PAGE_GUARD | PAGE_NOACCESS))) return "unavailable (not executable main-image code)";
        const auto offset = address - reinterpret_cast<std::uintptr_t>(region.BaseAddress);
        if (offset >= region.RegionSize) return "unavailable (range)";
        std::array<unsigned char, 192> bytes{};
        const auto count = (std::min)(bytes.size(), region.RegionSize - offset);
        SIZE_T read{};
        if (!ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address), bytes.data(), count, &read))
            return "unavailable (read failed)";
        constexpr std::string_view hex = "0123456789ABCDEF";
        std::string result;
        result.reserve(read * 2);
        for (std::size_t i = 0; i < read; ++i) {
            result += hex[bytes[i] >> 4];
            result += hex[bytes[i] & 15];
        }
        return result;
    }
}

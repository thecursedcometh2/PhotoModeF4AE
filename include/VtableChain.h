#pragma once
#include <windows.h>
#include <cstdint>
#include <optional>
#include <REL/Utility.h>
#include <atomic>

namespace Hooks
{
    inline std::optional<std::uintptr_t> ReadPointer(std::uintptr_t address)
    {
        std::uintptr_t result{};
        SIZE_T read{};
        if (!address || !ReadProcessMemory(GetCurrentProcess(), reinterpret_cast<const void*>(address),
                &result, sizeof(result), &read) || read != sizeof(result)) return std::nullopt;
        return result;
    }


    enum class ChainResult { installed, alreadyInstalled, replaced, invalid, writeFailed };

    // A later wrapper may already call us; re-chaining it would recurse.
    template <class Function>
    class VtableChain
    {
    public:
        VtableChain() = default;
        VtableChain(const VtableChain&) = delete;
        VtableChain& operator=(const VtableChain&) = delete;

        ChainResult Install(std::uintptr_t address, Function replacement)
        {
            const auto current = ReadPointer(address);
            const auto next = reinterpret_cast<std::uintptr_t>(replacement);
            if (!address || address % alignof(std::uintptr_t) || !current || !*current || !next)
                return ChainResult::invalid;
            if (slot) {
                if (slot != address || own != next) return ChainResult::invalid;
                return *current == own ? ChainResult::alreadyInstalled : ChainResult::replaced;
            }
            if (*current == next) return ChainResult::invalid;
            // Publish before exposing the wrapper.
            original.store(reinterpret_cast<Function>(*current), std::memory_order_release);
            slot = address;
            own = next;
            const bool written = REL::WriteSafeData(slot, own);
            if (!written || ReadPointer(slot) != own) return ChainResult::writeFailed;
            return ChainResult::installed;
        }

        Function Original() const { return original.load(std::memory_order_acquire); }
        bool OwnsSlot() const { return slot && ReadPointer(slot) == own; }

        bool Restore()
        {
            if (!OwnsSlot()) return false;
            return REL::WriteSafeData(slot, reinterpret_cast<std::uintptr_t>(Original()));
        }
    private:
        std::uintptr_t slot{}, own{};
        std::atomic<Function> original{};
    };
}

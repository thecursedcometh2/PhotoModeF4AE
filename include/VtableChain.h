#pragma once
#include "CameraTrace.h"
#include <REL/Utility.h>
#include <atomic>

namespace Hooks
{
    enum class ChainResult { installed, alreadyInstalled, replaced, invalid, writeFailed };

    // Capture the current downstream function once. Never capture our own hook
    // or re-chain a later wrapper: it could already delegate to us and recurse.
    template <class Function>
    class VtableChain
    {
    public:
        VtableChain() = default;
        VtableChain(const VtableChain&) = delete;
        VtableChain& operator=(const VtableChain&) = delete;

        ChainResult Install(std::uintptr_t address, Function replacement)
        {
            const auto current = CameraTrace::ReadPointer(address);
            const auto next = reinterpret_cast<std::uintptr_t>(replacement);
            if (!address || address % alignof(std::uintptr_t) || !current || !*current || !next)
                return ChainResult::invalid;
            if (slot) {
                if (slot != address || own != next) return ChainResult::invalid;
                return *current == own ? ChainResult::alreadyInstalled : ChainResult::replaced;
            }
            if (*current == next) return ChainResult::invalid;
            // Publish the downstream callable before making our wrapper visible.
            original.store(reinterpret_cast<Function>(*current), std::memory_order_release);
            slot = address;
            own = next;
            const bool written = REL::WriteSafeData(slot, own);
            if (!written || CameraTrace::ReadPointer(slot) != own) return ChainResult::writeFailed;
            return ChainResult::installed;
        }

        Function Original() const { return original.load(std::memory_order_acquire); }
        bool OwnsSlot() const { return slot && CameraTrace::ReadPointer(slot) == own; }

        // Only for explicit cleanup (fixtures). Do not erase a newer mod's slot.
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

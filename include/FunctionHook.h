#pragma once
#include <MinHook.h>

namespace Hooks
{
    // MinHook decodes/relocates whole instructions rather than assuming a
    // prologue length. Enable only after the original trampoline is available.
    template <class Function>
    class FunctionHook
    {
    public:
        FunctionHook() = default;
        FunctionHook(const FunctionHook&) = delete;
        FunctionHook& operator=(const FunctionHook&) = delete;

        MH_STATUS Install(void* address, Function detour)
        {
            if (target) return MH_ERROR_ALREADY_CREATED;
            auto status = MH_Initialize();
            if (status != MH_OK && status != MH_ERROR_ALREADY_INITIALIZED) return status;
            void* trampoline{};
            status = MH_CreateHook(address, reinterpret_cast<void*>(detour), &trampoline);
            if (status != MH_OK) return status;
            target = address;
            original = reinterpret_cast<Function>(trampoline);
            status = MH_EnableHook(target);
            if (status == MH_OK) enabled = true;
            else Remove();
            return status;
        }
        MH_STATUS Remove()
        {
            if (!target) return MH_ERROR_NOT_CREATED;
            const auto status = MH_RemoveHook(target);
            if (status == MH_OK) {
                target = nullptr;
                original = nullptr;
                enabled = false;
            }
            return status;
        }
        Function Original() const { return original; }
        bool Enabled() const { return enabled; }
    private:
        void* target{};
        Function original{};
        bool enabled{};
    };
}

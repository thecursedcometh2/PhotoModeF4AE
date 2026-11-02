#include "CameraMath.h"
#include "CameraTrace.h"
#include "VtableChain.h"
#include <atomic>
#include <iostream>
#include <stdexcept>

namespace
{
    void require(bool condition, const char* message)
    {
        if (!condition) throw std::runtime_error(message);
    }
    bool same(CameraMath::Quaternion a, CameraMath::Quaternion b)
    {
        return std::abs(a.w-b.w) < 1e-6f && std::abs(a.x-b.x) < 1e-6f &&
            std::abs(a.y-b.y) < 1e-6f && std::abs(a.z-b.z) < 1e-6f;
    }
    class CameraFixture
    {
    public:
        virtual __declspec(noinline) void GetRotation(CameraMath::Quaternion& out) const { out = base; }
        CameraMath::Quaternion base{0.5f, 0.5f, 0.5f, 0.5f};
    };
    using Getter = void (*)(const CameraFixture*, CameraMath::Quaternion&);
    __declspec(noinline) void dispatched(const CameraFixture& camera, CameraMath::Quaternion& out)
    {
        // Force a real vtable load; prevent compiler devirtualization in the fixture.
        const auto table = *reinterpret_cast<Getter const* const*>(&camera);
        Getter volatile entry = table[0];
        entry(&camera, out);
    }
    Hooks::VtableChain<Getter> samChain, vanillaChain;
    Hooks::VtableChain<Getter>* currentChain = &samChain;
    std::atomic<unsigned> earlyCalls{}, samCalls{}, photoCalls{}, newerCalls{};
    bool active{};
    float photoRoll{}, samRoll{0.3f};
    Getter newerDownstream{};
    void earlyPhoto(const CameraFixture* camera, CameraMath::Quaternion& out)
    {
        ++earlyCalls;
        out = camera->base;
    }
    void samGetter(const CameraFixture* camera, CameraMath::Quaternion& out)
    {
        ++samCalls;
        // SAM replaces the old getter instead of calling the earlier wrapper.
        out = CameraMath::ApplyLocalRoll(camera->base, samRoll);
    }
    void photoGetter(const CameraFixture* camera, CameraMath::Quaternion& out)
    {
        ++photoCalls;
        currentChain->Original()(camera, out);
        if (active) out = CameraMath::ApplyLocalRoll(out, photoRoll);
    }
    void newerGetter(const CameraFixture* camera, CameraMath::Quaternion& out)
    {
        ++newerCalls;
        newerDownstream(camera, out); // Already includes Photo Mode: do not re-chain it.
    }
}

int main()
{
    try {
        CameraFixture camera;
        const auto table = *reinterpret_cast<std::uintptr_t const* const*>(&camera);
        const auto slot = reinterpret_cast<std::uintptr_t>(table); // Fixture declares one virtual method.
        const auto vanilla = table[0];
        require(CameraTrace::VtableEntry(reinterpret_cast<std::uintptr_t>(&camera), 0) == vanilla, "live fixture vtable");
        require(!CameraTrace::ReadPointer(0) && !CameraTrace::ReadPointer(1), "invalid read");
        require(!CameraTrace::VtableEntry(reinterpret_cast<std::uintptr_t>(&camera), SIZE_MAX), "slot overflow");
        const auto description = CameraTrace::Describe(vanilla);
        require(description.find(".exe+") != description.npos && description.find('\\') == description.npos &&
            description.find('/') == description.npos, "image label must omit installation path");
        const auto code = CameraTrace::CodeBytes(vanilla);
        require(!code.empty() && code.size() <= 384 && code.find_first_not_of("0123456789ABCDEF") == code.npos, "bounded code read");
        require(CameraTrace::CodeBytes(reinterpret_cast<std::uintptr_t>(&camera)).starts_with("unavailable"), "stack read rejected");
        std::cout << "Camera trace checks passed: live vtable, invalid/overflow reads, basename-only labels, executable-only snapshots\n";

        // Reproduce the reported regression: SAM installs after an early Photo Mode wrapper.
        require(REL::WriteSafeData(slot, reinterpret_cast<std::uintptr_t>(&earlyPhoto)), "install early wrapper");
        require(REL::WriteSafeData(slot, reinterpret_cast<std::uintptr_t>(&samGetter)), "SAM replacement");
        CameraMath::Quaternion out{};
        dispatched(camera, out);
        require(samCalls == 1 && earlyCalls == 0, "reproduce overwritten early hook");
        require(samChain.Install(slot, &photoGetter) == Hooks::ChainResult::installed, "late installation");
        require(samChain.Original() == &samGetter && samChain.OwnsSlot(), "capture live SAM callback");
        dispatched(camera, out);
        require(same(out, CameraMath::ApplyLocalRoll(camera.base, samRoll)), "inactive preserves SAM rotation");
        active = true;
        photoRoll = 0.7f;
        const auto selectedRoll = photoRoll;
        for (int frame = 0; frame < 1000; ++frame) {
            require(samChain.Install(slot, &photoGetter) == Hooks::ChainResult::alreadyInstalled, "repeat install must not capture self");
            const auto beforeSAM = samCalls.load();
            const auto beforePhoto = photoCalls.load();
            dispatched(camera, out);
            const auto expected = CameraMath::ApplyLocalRoll(CameraMath::ApplyLocalRoll(camera.base, samRoll), photoRoll);
            require(same(out, expected), "roll missing, duplicated or accumulated");
            require(samCalls == beforeSAM + 1 && photoCalls == beforePhoto + 1, "downstream must run exactly once");
            require(photoRoll == selectedRoll, "selected roll changed");
        }
        // SAM changes its own camera while Photo Mode remains active.
        camera.base = {1, 0, 0, 0};
        samRoll = -0.4f;
        dispatched(camera, out);
        require(same(out, CameraMath::ApplyLocalRoll(CameraMath::ApplyLocalRoll(camera.base, samRoll), photoRoll)), "SAM camera change lost");
        active = false;
        dispatched(camera, out);
        require(same(out, CameraMath::ApplyLocalRoll(camera.base, samRoll)), "exit must remove only Photo Mode roll");

        newerDownstream = &photoGetter;
        require(REL::WriteSafeData(slot, reinterpret_cast<std::uintptr_t>(&newerGetter)), "later wrapper installation");
        require(samChain.Install(slot, &photoGetter) == Hooks::ChainResult::replaced, "do not re-chain later wrapper");
        require(samChain.Original() == &samGetter && !samChain.Restore(), "do not replace downstream or erase later owner");
        dispatched(camera, out);
        require(newerCalls == 1 && same(out, CameraMath::ApplyLocalRoll(camera.base, samRoll)), "later delegation recursed or changed output");
        require(REL::WriteSafeData(slot, reinterpret_cast<std::uintptr_t>(&photoGetter)), "restore fixture ownership");
        require(samChain.Restore() && CameraTrace::ReadPointer(slot) == reinterpret_cast<std::uintptr_t>(&samGetter), "cleanup must restore SAM, not vanilla");

        // No-SAM installation: same code captures vanilla and applies exactly one contribution.
        require(REL::WriteSafeData(slot, vanilla), "reset vanilla fixture");
        currentChain = &vanillaChain;
        require(vanillaChain.Install(slot, &photoGetter) == Hooks::ChainResult::installed, "vanilla chain installation");
        active = true;
        photoRoll = -1.5707963267948966f;
        dispatched(camera, out);
        require(same(out, CameraMath::ApplyLocalRoll(camera.base, photoRoll)), "vanilla roll");
        photoRoll = 0;
        dispatched(camera, out);
        require(same(out, camera.base), "zero/reset restoration");
        require(vanillaChain.Restore() && CameraTrace::ReadPointer(slot) == vanilla, "restore vanilla fixture");
        std::cout << "Late camera chain checks passed: reproduced SAM overwrite, captured SAM getter, 1000 idempotent frames, one contribution, SAM changes, exit baseline, later-wrapper recursion protection, vanilla fallback\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

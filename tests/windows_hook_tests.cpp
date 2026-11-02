#include "CameraMath.h"
#include "FunctionHook.h"
#include <atomic>
#include <cmath>
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
    // These two paths deliberately exercise a direct CALL and vtable dispatch
    // against the exact same entry, reproducing the old hook's blind spot.
    __declspec(noinline) void direct(const CameraFixture& camera, CameraMath::Quaternion& out)
    {
        camera.CameraFixture::GetRotation(out);
    }
    __declspec(noinline) void dispatched(const CameraFixture& camera, CameraMath::Quaternion& out)
    {
        camera.GetRotation(out);
    }
    using GetRotation = void (*)(const CameraFixture*, CameraMath::Quaternion&);
    Hooks::FunctionHook<GetRotation> hook;
    std::atomic<unsigned> calls{};
    bool active{};
    float roll{};
    void detour(const CameraFixture* camera, CameraMath::Quaternion& out)
    {
        ++calls;
        hook.Original()(camera, out);
        if (active) out = CameraMath::ApplyLocalRoll(out, roll);
    }
}

int main()
{
    try {
        CameraFixture camera;
        auto* target = (*reinterpret_cast<void***>(&camera))[0]; // Fixture declares exactly one virtual method.
        const auto status = hook.Install(target, &detour);
        require(status == MH_OK && hook.Enabled(), MH_StatusToString(status));
        CameraMath::Quaternion out{};
        direct(camera, out);
        require(same(out, camera.base) && calls == 1, "inactive direct call/original trampoline");
        dispatched(camera, out);
        require(same(out, camera.base) && calls == 2, "inactive virtual call");
        active = true;
        roll = 0.7f;
        const auto expected = CameraMath::ApplyLocalRoll(camera.base, roll);
        for (int frame = 0; frame < 1000; ++frame) {
            direct(camera, out);
            require(same(out, expected), "direct call roll missing or accumulated");
            dispatched(camera, out);
            require(same(out, expected), "virtual call roll missing or accumulated");
        }
        require(calls == 2002, "entry hook missed a direct or virtual call");
        require(same(camera.base, {0.5f, 0.5f, 0.5f, 0.5f}), "stored camera rotation modified");
        // Negative and zero settings, then exit: output always derives from
        // the current original rotation, including another camera writer.
        camera.base = {1, 0, 0, 0};
        roll = -1.5707963267948966f;
        direct(camera, out);
        require(same(out, CameraMath::ApplyLocalRoll(camera.base, roll)), "negative roll or newer baseline lost");
        roll = 0;
        dispatched(camera, out);
        require(same(out, camera.base), "zero roll not restored");
        roll = 0.7f;
        active = false;
        direct(camera, out);
        require(same(out, camera.base), "roll persisted after Photo Mode exit");
        require(hook.Remove() == MH_OK && !hook.Enabled(), "remove hook");
        const auto previous = calls.load();
        direct(camera, out);
        require(calls == previous && same(out, camera.base), "entry not restored after removal");
        std::cout << "Entry hook checks passed: direct/virtual calls, original trampoline, 1000 frames without accumulation, changed baseline, zero/exit restoration, removal\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}

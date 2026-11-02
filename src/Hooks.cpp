#include "Hooks.h"
#include "CameraMath.h"
#include "CameraTrace.h"
#include "FunctionHook.h"
#include <atomic>

#include "Input.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

namespace PhotoMode
{
    namespace
    {
        std::atomic<std::uint64_t> rotationCalls{}, rollApplications{};
        std::atomic<bool> traceEmitted{};
        std::string originalGetterCode;
    }

    struct FreeCameraRotation
    {
        static void thunk(const RE::FreeCameraState* state, RE::NiQuaternion& rotation)
        {
            ++rotationCalls;
            hook.Original()(state, rotation);
            const auto mode = MANAGER(PhotoMode);
            const auto camera = RE::PlayerCamera::GetSingleton();
            if (!mode->IsActive() || !camera || camera->currentState.get() != state) return;
            const float roll = mode->GetViewRoll(0.0f);
            if (!std::isfinite(roll) || roll == 0.0f) return;
            // SAM suspends our input, not the selected camera roll. Compose
            // against the original result on every call, never stored angles.
            const auto result = CameraMath::ApplyLocalRoll(
                {rotation.w, rotation.x, rotation.y, rotation.z}, roll);
            rotation = {result.w, result.x, result.y, result.z};
            if (++rollApplications == 1) logger::info("View roll applied through FreeCameraState::GetRotation entry hook");
        }
        static inline Hooks::FunctionHook<decltype(&thunk)> hook;

        static void Install()
        {
            // This function ID is from the authoritative original Hooks.cpp.
            // Verify it agrees with the declared getter slot before patching.
            REL::Relocation<std::uintptr_t> target{ REL::ID(2248281) };
            REL::Relocation<std::uintptr_t> table{ RE::VTABLE::FreeCameraState[0] };
            const auto entry = reinterpret_cast<const std::uintptr_t*>(table.address())[0x0C];
            const auto base = REX::FModule::GetExecutingModule().GetBaseAddress();
            if (entry != target.address()) {
                logger::error("View roll entry hook not installed: GetRotation ID/vtable mismatch (ID RVA={:X}, slot RVA={:X}); requires binary analysis", target.address() - base, entry - base);
                return;
            }
            originalGetterCode = CameraTrace::CodeBytes(target.address());
            const auto status = hook.Install(reinterpret_cast<void*>(target.address()), &thunk);
            if (status != MH_OK) {
                logger::error("View roll entry hook failed at RVA {:X}: {}", target.address() - base, MH_StatusToString(status));
                return;
            }
            logger::info("View roll entry hook installed at RVA {:X}; roll remains active while SAM is open", target.address() - base);
        }
    };

    static void LogLiveCameraTrace()
    {
        if (traceEmitted.exchange(true)) return;
        logger::info("Roll trace BEGIN (2.1.6 diagnostic; no new camera hook)");
        const auto mode = MANAGER(PhotoMode);
        logger::info("Roll trace mode: active={}, SAM={}, selected radians={}",
            mode->IsActive(), mode->IsScreenArcherMenuOpen(), mode->GetViewRoll(0.0f));
        const auto camera = RE::PlayerCamera::GetSingleton();
        if (!camera) {
            logger::info("Roll trace player camera unavailable");
            logger::info("Roll trace END");
            return;
        }
        const auto describeObject = [](const char* name, std::uintptr_t object, std::uintptr_t expected) {
            const auto table = CameraTrace::ReadPointer(object);
            logger::info("Roll trace {} vtable: live={}, expected={}, matches={}", name,
                table ? CameraTrace::Describe(*table) : "unreadable", CameraTrace::Describe(expected),
                table && *table == expected);
        };
        const auto describeSlot = [](const char* name, std::uintptr_t object, std::size_t slot) {
            const auto function = CameraTrace::VtableEntry(object, slot);
            logger::info("Roll trace {} slot {:X}: {}", name, slot,
                function ? CameraTrace::Describe(*function) : "unreadable");
            if (function) logger::info("Roll trace code {}: {}", name, CameraTrace::CodeBytes(*function));
        };
        REL::Relocation<std::uintptr_t> expectedPlayer{ RE::VTABLE::PlayerCamera[0] };
        REL::Relocation<std::uintptr_t> expectedFree{ RE::VTABLE::FreeCameraState[0] };
        REL::Relocation<std::uintptr_t> expectedRender{ RE::VTABLE::NiCamera[0] };
        const auto object = reinterpret_cast<std::uintptr_t>(camera);
        describeObject("PlayerCamera", object, expectedPlayer.address());
        describeSlot("PlayerCamera.Update", object, 0x03); // TESCamera.h
        const auto state = camera->currentState;
        if (state) {
            logger::info("Roll trace current state ID={} (expected free={})",
                std::to_underlying(state->id.get()), std::to_underlying(RE::CameraState::kFree));
            const auto stateObject = reinterpret_cast<std::uintptr_t>(state.get());
            describeObject("current state", stateObject, expectedFree.address());
            describeSlot("state.Update", stateObject, 0x0B); // TESCameraState.h
            describeSlot("state.GetRotation", stateObject, 0x0C);
            describeSlot("state.GetTranslation", stateObject, 0x0D);
        }
        REL::Relocation<std::uintptr_t> knownGetter{ REL::ID(2248281) };
        logger::info("Roll trace ID2248281: {}, pre-detour code={}",
            CameraTrace::Describe(knownGetter.address()), originalGetterCode);
        if (const auto graphics = RE::BSGraphics::State::GetSingleton()) {
            const auto renderCamera = CameraTrace::ReadPointer(
                reinterpret_cast<std::uintptr_t>(std::addressof(graphics->cameraState.referenceCamera)));
            if (renderCamera && *renderCamera) {
                describeObject("render camera", *renderCamera, expectedRender.address());
                describeSlot("render.UpdateWorldData", *renderCamera, 0x34); // NiCamera.h
            } else logger::info("Roll trace render camera unavailable");
        }
        logger::info("Roll trace END");
    }

    void LogViewRollStatus(float degrees)
    {
        logger::info("View roll changed to {} degrees; entry hook={}, rotation calls={}, roll applications={}",
            degrees, FreeCameraRotation::hook.Enabled(), rotationCalls.load(), rollApplications.load());
        LogLiveCameraTrace();
    }

	struct DOF_IsActive
	{
		static bool thunk(RE::ImageSpaceEffectDepthOfField* a_this)
		{
			if (MANAGER(PhotoMode)->IsActive()) {
				return false;
			}
			return func(a_this);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x8 };
	};

	struct Bokeh_IsActive
	{
		static bool thunk(RE::ImageSpaceEffectDepthOfField* a_this)
		{
			if (MANAGER(PhotoMode)->IsActive()) {
				return MANAGER(PhotoMode)->GetDOFEnabled();
			}
			return func(a_this);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x8 };
	};

	struct CallParams
	{
		static void thunk(RE::PauseMenu* a_this, const Scaleform::GFx::FunctionHandler::Params& a_params)
		{
			auto id = reinterpret_cast<std::uintptr_t>(a_params.userData);
			if (id == 16 && MANAGER(PhotoMode)->CanActivateFromPauseMenu()) {
				if (auto& movie = a_this->uiMovie) {
					Scaleform::GFx::Value selectedEntry;
					if (movie->GetVariable(&selectedEntry, "root1.Menu_mc.MainPanel_mc.List_mc.selectedEntry")) {
						Scaleform::GFx::Value entryMarker;
						Scaleform::GFx::Value entryState;
						if (auto result = selectedEntry.GetMember("po3PhotoModeEntry", &entryMarker); result && entryMarker.GetBoolean()) {
							if (result = selectedEntry.GetMember("disabled", &entryState); result && !entryState.GetBoolean()) {
								MANAGER(PhotoMode)->ActivateFromPauseMenu();
							}
							return;
						}
					}
				}
			}

			return func(a_this, a_params);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x1 };
	};

	struct MenuOpenHandler_OnButtonEvent
	{
		static void thunk(RE::MenuOpenHandler* a_this, RE::ButtonEvent* a_event)
		{
			func(a_this, a_event);

			if (a_event->QUserEvent() == "VATS" && MANAGER(PhotoMode)->IsActive() && !PhotoMode::Manager::IsScreenArcherMenuOpen()) {
				a_event->handled = RE::InputEvent::HANDLED_RESULT::kContinue;
			}
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x8 };
	};

	// TESDataHandler idle array is not populated
	struct SetFormEditorID
	{
		static bool thunk(RE::TESIdleForm* a_this, const char* a_str)
		{
			if (!clib_util::string::is_empty(a_str)) {
				if (const std::string str(a_str); !str.starts_with("pa_")) {  // paired anims
					cachedIdles.emplace(a_str, a_this);
				}
			}
			return func(a_this, a_str);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline constexpr std::size_t            idx{ 0x3B };
	};

	void InstallHooks()
	{
		FreeCameraRotation::Install();

		stl::write_vfunc<RE::ImageSpaceEffectBokehDepthOfField, Bokeh_IsActive>();
		stl::write_vfunc<RE::ImageSpaceEffectDepthOfField, DOF_IsActive>();

		stl::write_vfunc<RE::PauseMenu, CallParams>();
		// This revision declares MenuOpenHandler::VTABLE privately. Use the
        // same public generated vtable registry entry, without changing its ID.
        REL::Relocation<std::uintptr_t> menuOpenVtable{ RE::VTABLE::MenuOpenHandler[0] };
        MenuOpenHandler_OnButtonEvent::func = menuOpenVtable.write_vfunc(
            MenuOpenHandler_OnButtonEvent::idx, MenuOpenHandler_OnButtonEvent::thunk);

		stl::write_vfunc<RE::TESIdleForm, SetFormEditorID>();
	}
}

namespace Screenshot
{
	struct TakeScreenshot
	{
		static void thunk(char const* a_path, RE::BSGraphics::TextureFileFormat a_format, std::uint32_t a_renderTarget)
		{
            const bool queued = MANAGER(Input)->IsScreenshotQueued();
            bool saved = false;
            if (queued) {
                saved = MANAGER(Screenshot)->TakeScreenshot();
            } else {
                func(a_path, a_format, a_renderTarget);
            }
            MANAGER(Input)->OnScreenshotFinish();
            if (queued) {
                // Never fall through to an unchecked writer after a custom save
                // failure: it could reuse the same stale vanilla index.
                RE::SendHUDMessage::ShowHUDMessage(saved ? "$PM_ScreenshotNotif"_T : "$PM_ScreenshotFailed"_T, nullptr, true, false);
                if (!saved) logger::error("Photo Mode screenshot was not saved; vanilla fallback suppressed to protect existing photos");
            }
		}
		static inline REL::Relocation<decltype(thunk)> func;
	};

	void InstallHooks()
	{
		REL::Relocation<std::uintptr_t> target{ REL::ID(2228913) };  // Main::Swap; target-runtime offset remains unverified
		stl::write_thunk_call<TakeScreenshot>(target.address() + 0x2C6);
	}
}

void Hooks::Install()
{
	PhotoMode::InstallHooks();
	Screenshot::InstallHooks();
}

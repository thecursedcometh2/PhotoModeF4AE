#include "Hooks.h"
#include "CameraMath.h"
#include "VtableChain.h"

#include "Input.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

namespace PhotoMode
{
    struct FreeCameraRotation
    {
        static void thunk(const RE::FreeCameraState* state, RE::NiQuaternion& rotation)
        {
            hook.Original()(state, rotation);
            const auto mode = MANAGER(PhotoMode);
            const auto camera = RE::PlayerCamera::GetSingleton();
            if (!mode->IsActive() || !camera || camera->currentState.get() != state) return;
            const float roll = mode->GetViewRoll(0.0f);
            if (!std::isfinite(roll) || roll == 0.0f) return;
            const auto result = CameraMath::ApplyLocalRoll(
                {rotation.w, rotation.x, rotation.y, rotation.z}, roll);
            rotation = {result.w, result.x, result.y, result.z};
        }
        static inline Hooks::VtableChain<decltype(&thunk)> hook;
        static inline bool reportedFailure{};

        static void Install()
        {
            const auto camera = RE::PlayerCamera::GetSingleton();
            if (!camera || !camera->currentState || camera->currentState->id != RE::CameraState::kFree) return;
            const auto live = Hooks::ReadPointer(reinterpret_cast<std::uintptr_t>(camera->currentState.get()));
            static REL::Relocation<std::uintptr_t> expected{ RE::VTABLE::FreeCameraState[0] };
            if (!live || *live != expected.address()) {
                if (!reportedFailure) logger::error("View roll unavailable: unknown camera vtable.");
                reportedFailure = true;
                return;
            }
            constexpr std::size_t slot = 0x0C;
            const auto result = hook.Install(*live + slot * sizeof(void*), &thunk);
            if (result != Hooks::ChainResult::installed && result != Hooks::ChainResult::alreadyInstalled && !reportedFailure) {
                logger::error("View roll chain failed ({}). Skipping a later hook to avoid recursion.",
                    std::to_underlying(result));
                reportedFailure = true;
            }
        }
    };

    void EnsureViewRollHook()
    {
        FreeCameraRotation::Install();
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

		stl::write_vfunc<RE::ImageSpaceEffectBokehDepthOfField, Bokeh_IsActive>();
		stl::write_vfunc<RE::ImageSpaceEffectDepthOfField, DOF_IsActive>();

		stl::write_vfunc<RE::PauseMenu, CallParams>();
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
                if (!saved) logger::error("Photo not saved. Fallback skipped to protect existing photos.");
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

#include "Hooks.h"
#include "CameraMath.h"

#include "Input.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

namespace PhotoMode
{
    // Intercept the public camera-state rotation result rather than a call
    // instruction inside GetRotation. Slot 0x0C is declared in TESCameraState.
    struct FreeCameraGetRotation
    {
        static void thunk(const RE::FreeCameraState* state, RE::NiQuaternion& rotation)
        {
            func(state, rotation);
            if (!MANAGER(PhotoMode)->IsActive()) return;
            const float roll = MANAGER(PhotoMode)->GetViewRoll(0.0f);
            if (roll == 0.0f) return;
            const auto result = CameraMath::ApplyLocalRoll(
                {rotation.w, rotation.x, rotation.y, rotation.z}, roll);
            rotation = {result.w, result.x, result.y, result.z};
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static inline constexpr std::size_t idx = 0x0C;
    };

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
						Scaleform::GFx::Value entryText;
						Scaleform::GFx::Value entryState;
						if (auto result = selectedEntry.GetMember("text", &entryText); result && std::string(entryText.GetString()) == "$PM_Title_Menu") {
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

			if (a_event->QUserEvent() == "VATS" && MANAGER(PhotoMode)->IsActive()) {
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
		stl::write_vfunc<RE::FreeCameraState, FreeCameraGetRotation>();

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
			bool skipVanillaScreenshot = false;

			if (MANAGER(Input)->IsScreenshotQueued()) {
				skipVanillaScreenshot = MANAGER(Screenshot)->TakeScreenshot();
			}

			if (!skipVanillaScreenshot) {
				func(a_path, a_format, a_renderTarget);
			}

			MANAGER(Input)->OnScreenshotFinish();

			if (skipVanillaScreenshot) {
				RE::SendHUDMessage::ShowHUDMessage("$PM_ScreenshotNotif"_T, nullptr, true, false);
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

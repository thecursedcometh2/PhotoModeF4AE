#include "Hooks.h"
#include "CameraRoll.h"
#include <atomic>

#include "Input.h"
#include "PhotoMode/Hotkeys.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"

namespace PhotoMode
{
    namespace
    {
        RE::NiPointer<RE::NiNode> rolledRoot;
        CameraMath::RollOverlay rollOverlay;
        std::atomic<std::uint64_t> cameraUpdates{}, rollApplications{};

        CameraMath::Rotation ReadRotation(const RE::NiMatrix3& rotation)
        {
            CameraMath::Rotation result{};
            for (std::size_t row = 0; row < 3; ++row)
                for (std::size_t col = 0; col < 3; ++col) result[row][col] = rotation[row][col];
            return result;
        }

        void WriteRotation(RE::NiNode& root, const CameraMath::Rotation& rotation)
        {
            auto local = root.GetLocalRotate();
            for (std::size_t row = 0; row < 3; ++row)
                for (std::size_t col = 0; col < 3; ++col) local[row][col] = rotation[row][col];
            root.SetLocalRotate(local);
            // Propagate to the child NiCamera and its view transform. Preserve
            // translation/scale and do not edit FreeCameraState pitch/yaw.
            RE::NiUpdateData data{};
            root.Update(data);
        }
    }

    void ReleaseViewRoll()
    {
        if (!rolledRoot) return;
        auto rotation = ReadRotation(rolledRoot->GetLocalRotate());
        if (rollOverlay.Release(rolledRoot.get(), rotation)) WriteRotation(*rolledRoot, rotation);
        rolledRoot.reset();
    }

    void LogViewRollStatus(float degrees)
    {
        logger::info("View roll changed to {} degrees; camera updates={}, roll applications={}",
            degrees, cameraUpdates.load(), rollApplications.load());
    }

    // Use the public TESCamera update boundary. A GetRotation vtable hook can
    // be bypassed by direct calls or another camera implementation.
    struct PlayerCameraUpdate
    {
        static void thunk(RE::PlayerCamera* camera)
        {
            ++cameraUpdates;
            ReleaseViewRoll();
            func(camera);
            const auto mode = MANAGER(PhotoMode);
            if (!mode->IsActive() || mode->IsScreenArcherMenuOpen() ||
                !camera->currentState || camera->currentState->id != RE::CameraState::kFree ||
                !camera->cameraRoot) return;
            const float roll = mode->GetViewRoll(0.0f);
            if (!std::isfinite(roll) || roll == 0.0f) return;
            rolledRoot = camera->cameraRoot;
            const auto rotation = rollOverlay.Apply(rolledRoot.get(), ReadRotation(rolledRoot->GetLocalRotate()), roll);
            WriteRotation(*rolledRoot, rotation);
            if (++rollApplications == 1) logger::info("View roll applied through PlayerCamera::Update");
        }
        static inline REL::Relocation<decltype(thunk)> func;
        static inline constexpr std::size_t idx = 0x03; // TESCamera::Update, pinned header
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
		stl::write_vfunc<RE::PlayerCamera, PlayerCameraUpdate>();

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

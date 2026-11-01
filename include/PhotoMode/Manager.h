#pragma once

#include "ImGui/Fonts/IconsFontAwesome6.h"

#include "Tabs/Camera.h"
#include "Tabs/Character.h"
#include "Tabs/Filters.h"
#include "Tabs/Overlays.h"
#include "Tabs/Time.h"

namespace PhotoMode
{
	class Manager :
		public REX::TSingleton<Manager>,
		public RE::BSTEventSink<RE::MenuOpenCloseEvent>
	{
	public:
		static void Register();
		void        LoadMCMSettings(const CSimpleIniA& a_ini);

		static bool        IsValid();
		bool               ShouldBlockInput() const;
		[[nodiscard]] bool IsActive() const;
		void               Activate();
		void               Deactivate();
		bool               ToggleActive();

		bool CanActivateFromPauseMenu() const;
		void ActivateFromPauseMenu();

		void Revert(bool a_deactivate = false);
		void QuitOnEscape();

		bool GetResetAll() const;
		void DoResetAll();

		[[nodiscard]] bool IsHidden() const;
		void               ToggleUI();

		void NavigateTab(bool a_left);

		[[nodiscard]] float GetViewRoll(float a_fallback) const;
		[[nodiscard]] bool  GetDOFEnabled() const;

		void Draw();
		bool OnFrameUpdate();

		void                              OnDataLoad();
		std::pair<ImGui::Texture*, float> GetOverlay() const;

	private:
        bool originalSavingAllowed{ true };
        std::array<bool, 5> originalHandlerStates{};
        std::array<RE::BSInputEventUser*, 5> originalHandlers{};
		enum TAB_TYPE : std::int32_t
		{
			kCamera,
			kTime,
			kCharacter,
			kFilters,
			kOverlays
		};

		static constexpr std::array tabs = {
			"$PM_Camera",
			"$PM_TimeWeather",
			"$PM_Player",
			"$PM_Filters",
			"$PM_Overlays"
		};
		static constexpr std::array tabIcons = {
			ICON_FA_CAMERA_RETRO,
			ICON_FA_CLOCK,
			ICON_FA_PERSON,
			ICON_FA_CIRCLE_HALF_STROKE,
			ICON_FA_IMAGE
		};

		static constexpr std::array tabResetNotifs = { "$PM_ResetNotifCamera", "$PM_ResetNotifTime", "$PM_ResetNotifPlayer", "$PM_ResetNotifFilters", "$PM_ResetNotifOverlays" };

		void               TogglePlayerControls(bool a_enable);
		void               DrawControls();
		void               DrawBar() const;
		[[nodiscard]] bool SetupPauseMenu() const;

		EventResult ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*) override;

		// members
		bool activated{ false };
		bool hiddenUI{ false };
		bool blockInput{ false };

		REX::TEnumSet<RE::UserEvents::USER_EVENT_FLAG, std::uint32_t> controlFlags{
			RE::UserEvents::USER_EVENT_FLAG::kActivate,
			RE::UserEvents::USER_EVENT_FLAG::kMenu,
			RE::UserEvents::USER_EVENT_FLAG::kVATS
		};
		RE::BSTSmartPointer<RE::BSInputEnableLayer> inputLayer{};

		std::int32_t previousTab{ kCamera };
		std::int32_t currentTab{ kCamera };

		Camera cameraTab;
		Time   timeTab;

		Map<RE::FormID, Character> characterTab;
		RE::Actor*                 cachedCharacter{ nullptr };
		RE::Actor*                 prevCachedCharacter{ nullptr };

		Filters  filterTab;
		Overlays overlaysTab;

		bool updateKeyboardFocus{ false };

		RE::CameraState originalcameraState{ RE::CameraState::k3rdPerson };

		bool resetWindow{ true };
		bool resetPlayerTabs{ true };
		bool resetAll{ false };

		bool menusAlreadyHidden{ false };
		bool allowTextInput{ false };

		bool    noItemsFocused{ false };
		ImGuiID lastFocusedID{ 0 };
		bool    restoreLastFocusID{ false };

		float freeCameraSpeed{ 4.0f };
		bool  freezeTimeOnStart{ true };
		bool  openFromPauseMenu{ true };

		bool sitWaitMenuOpen{ false };

		RE::TESGlobal* activeGlobal{ nullptr };
	};
}

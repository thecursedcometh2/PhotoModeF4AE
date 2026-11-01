#pragma once

namespace Input
{
	enum class DEVICE
	{
		kNone,
		kKeyboard,
		kMouse,
		kGamepadDirectX,  // xbox
		kGamepadOrbis     // ps4
	};

	class Manager final :
		public REX::TSingleton<Manager>,
		public RE::BSInputEventUser
	{
	public:
		static void Register();
		void        LoadMCMSettings(const CSimpleIniA& a_ini);

		DEVICE GetInputDevice() const;
		void   ResetInputDevices();

		void HideMenu(bool a_hide);
		bool IsScreenshotQueued() const;
		void QueueScreenshot(bool a_forceQueue);
		void OnScreenshotFinish();

	private:
		bool SetInputDevice(RE::INPUT_DEVICE a_device);
		bool GetHotKey(RE::INPUT_DEVICE a_device, std::uint32_t& a_hotkey);

		static ImGuiKey                  ToImGuiKey(RE::BS_BUTTON_CODE a_key);
		static ImGuiKey                  ToImGuiKey_Mouse(RE::BS_BUTTON_CODE a_key);
		static std::pair<ImGuiKey, bool> ToImGuiKey_DirectX(RE::BS_BUTTON_CODE a_key);
		static std::pair<ImGuiKey, bool> ToImGuiKey_Orbis(RE::BS_BUTTON_CODE a_key);

		void SendKeyEvent(RE::BS_BUTTON_CODE a_key, float a_value, bool a_keyPressed) const;

		bool ShouldHandleEvent(const RE::InputEvent*) override;               // 01
		void OnThumbstickEvent(const RE::ThumbstickEvent* a_event) override;  // 04
		void OnMouseMoveEvent(const RE::MouseMoveEvent* a_event) override;    // 06
		void OnCharacterEvent(const RE::CharacterEvent* a_event) override;    // 07
		void OnButtonEvent(const RE::ButtonEvent* a_event) override;          // 08

		// members
		DEVICE           inputDevice{ DEVICE::kNone };
		DEVICE           lastInputDevice{ DEVICE::kNone };
		bool             screenshotQueued{ false };
		bool             menuHidden{ false };
		float            keyHeldDuration{ 0.5 };
	};
}

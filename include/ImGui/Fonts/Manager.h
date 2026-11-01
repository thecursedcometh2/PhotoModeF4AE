#pragma once

namespace Font
{
	struct Font
	{
		void LoadSettings(const CSimpleIniA& a_ini, const char* a_section);

		void LoadFont(bool a_loadIcons);

		ImFont* GetFont() const;

		// members
		std::string             name{ "Roboto-Regular.ttf" };
		std::pair<float, float> fontSize{ 32.0f, 36.0f };
		std::pair<float, float> iconSize{ 32.0f, 36.0f };
		ImFont*                 font{ nullptr };
	};

	using Button = std::pair<std::string, bool>;

	struct GamePadButton
	{
		std::string xbox;
		std::string ps4;
	};

	class Manager final : public REX::TSingleton<Manager>
	{
	public:
		void LoadSettings();
		void LoadMCMSettings(const CSimpleIniA& a_ini);

		void ReloadFonts();

		std::pair<float, float> GetDefaultFontSize() const;
		std::pair<float, float> GetDefaultIconSize() const;
		ImFont*                 GetControllerButtonFont() const;

		Button GetIcon(std::uint32_t key);
		Button GetGamePadIcon(const GamePadButton& a_icons) const;

	private:
		enum class BUTTON_SCHEME
		{
			kAutoDetect,
			kXbox,
			kPS4
		};

		// members
		bool loadedFonts{ false };

		Font defaultFont;
		Font controllerButtonFont;

		std::string unknownKey{ "?" };
		std::string unknownKeyController{ "?" };

		Map<RE::BS_BUTTON_CODE, std::string> keyboard{
			{ RE::BS_BUTTON_CODE::kTab, "Tab)" },
			{ RE::BS_BUTTON_CODE::kPageUp, "PgUp)" },
			{ RE::BS_BUTTON_CODE::kPageDown, "PgDn)" },
			{ RE::BS_BUTTON_CODE::kHome, "Home)" },
			{ RE::BS_BUTTON_CODE::kEnd, "End)" },
			{ RE::BS_BUTTON_CODE::kInsert, "Insert)" },
			{ RE::BS_BUTTON_CODE::kDelete, "Delete)" },
			{ RE::BS_BUTTON_CODE::kBackspace, "Backspace)" },
			{ RE::BS_BUTTON_CODE::kSpace, "Space)" },
			{ RE::BS_BUTTON_CODE::kEnter, "Enter)" },
			{ RE::BS_BUTTON_CODE::kEscape, "Esc)" },
			{ RE::BS_BUTTON_CODE::kLeft, "Left)" },
			{ RE::BS_BUTTON_CODE::kRight, "Right)" },
			{ RE::BS_BUTTON_CODE::kUp, "Up)" },
			{ RE::BS_BUTTON_CODE::kDown, "Down)" },
			{ RE::BS_BUTTON_CODE::kLControl, "L Ctrl)" },
			{ RE::BS_BUTTON_CODE::kLShift, "L Shift)" },
			{ RE::BS_BUTTON_CODE::kLAlt, "L Alt)" },
			//{ RE::BS_BUTTON_CODE::kLeftWin, "L Win)"},
			{ RE::BS_BUTTON_CODE::kRControl, "R Ctrl)" },
			{ RE::BS_BUTTON_CODE::kRShift, "R Shift)" },
			{ RE::BS_BUTTON_CODE::kRAlt, "R Alt)" },
			//{ RE::BS_BUTTON_CODE::kRightWin, "R Win)"},
			{ RE::BS_BUTTON_CODE::k0, "0)" },
			{ RE::BS_BUTTON_CODE::k1, "1)" },
			{ RE::BS_BUTTON_CODE::k2, "2)" },
			{ RE::BS_BUTTON_CODE::k3, "3)" },
			{ RE::BS_BUTTON_CODE::k4, "4)" },
			{ RE::BS_BUTTON_CODE::k5, "5)" },
			{ RE::BS_BUTTON_CODE::k6, "6)" },
			{ RE::BS_BUTTON_CODE::k7, "7)" },
			{ RE::BS_BUTTON_CODE::k8, "8)" },
			{ RE::BS_BUTTON_CODE::k9, "9)" },
			{ RE::BS_BUTTON_CODE::kA, "A)" },
			{ RE::BS_BUTTON_CODE::kB, "B)" },
			{ RE::BS_BUTTON_CODE::kC, "C)" },
			{ RE::BS_BUTTON_CODE::kD, "D)" },
			{ RE::BS_BUTTON_CODE::kE, "E)" },
			{ RE::BS_BUTTON_CODE::kF, "F)" },
			{ RE::BS_BUTTON_CODE::kG, "G)" },
			{ RE::BS_BUTTON_CODE::kH, "H)" },
			{ RE::BS_BUTTON_CODE::kI, "I)" },
			{ RE::BS_BUTTON_CODE::kJ, "J)" },
			{ RE::BS_BUTTON_CODE::kK, "K)" },
			{ RE::BS_BUTTON_CODE::kL, "L)" },
			{ RE::BS_BUTTON_CODE::kM, "M)" },
			{ RE::BS_BUTTON_CODE::kN, "N)" },
			{ RE::BS_BUTTON_CODE::kO, "O)" },
			{ RE::BS_BUTTON_CODE::kP, "P)" },
			{ RE::BS_BUTTON_CODE::kQ, "Q)" },
			{ RE::BS_BUTTON_CODE::kR, "R)" },
			{ RE::BS_BUTTON_CODE::kS, "S)" },
			{ RE::BS_BUTTON_CODE::kT, "T)" },
			{ RE::BS_BUTTON_CODE::kU, "U)" },
			{ RE::BS_BUTTON_CODE::kV, "V)" },
			{ RE::BS_BUTTON_CODE::kW, "W)" },
			{ RE::BS_BUTTON_CODE::kX, "X)" },
			{ RE::BS_BUTTON_CODE::kY, "Y)" },
			{ RE::BS_BUTTON_CODE::kZ, "Z)" },
			{ RE::BS_BUTTON_CODE::kF1, "F1)" },
			{ RE::BS_BUTTON_CODE::kF2, "F2)" },
			{ RE::BS_BUTTON_CODE::kF3, "F3)" },
			{ RE::BS_BUTTON_CODE::kF4, "F4)" },
			{ RE::BS_BUTTON_CODE::kF5, "F5)" },
			{ RE::BS_BUTTON_CODE::kF6, "F6)" },
			{ RE::BS_BUTTON_CODE::kF7, "F7)" },
			{ RE::BS_BUTTON_CODE::kF8, "F8)" },
			{ RE::BS_BUTTON_CODE::kF9, "F9)" },
			{ RE::BS_BUTTON_CODE::kF10, "F10)" },
			{ RE::BS_BUTTON_CODE::kF11, "F11)" },
			{ RE::BS_BUTTON_CODE::kF12, "F12)" },
			{ RE::BS_BUTTON_CODE::kApostrophe, "')" },
			{ RE::BS_BUTTON_CODE::kComma, ",)" },
			{ RE::BS_BUTTON_CODE::kMinus, "-)" },
			{ RE::BS_BUTTON_CODE::kPeriod, ".)" },
			//{ RE::BS_BUTTON_CODE::kSlash, "/)"},
			{ RE::BS_BUTTON_CODE::kSemicolon, ";)" },
			{ RE::BS_BUTTON_CODE::kEquals, "=)" },
			{ RE::BS_BUTTON_CODE::kLBracket, "[)" },
			{ RE::BS_BUTTON_CODE::kBackslash, "\\)" },
			{ RE::BS_BUTTON_CODE::kRBracket, "])" },
			//{ RE::BS_BUTTON_CODE::kTilde, "Tilde)"},
			{ RE::BS_BUTTON_CODE::kCapsLock, "CapsLock)" },
			{ RE::BS_BUTTON_CODE::kScrollLock, "ScrollLock)" },
			{ RE::BS_BUTTON_CODE::kNumLock, "NumLock)" },
			{ RE::BS_BUTTON_CODE::kPrintScreen, "PrtScn)" },
			{ RE::BS_BUTTON_CODE::kPause, "Pause)" },
			{ RE::BS_BUTTON_CODE::kNumpad_0, "NumPad0)" },
			{ RE::BS_BUTTON_CODE::kNumpad_1, "Keypad1)" },
			{ RE::BS_BUTTON_CODE::kNumpad_2, "Keypad2)" },
			{ RE::BS_BUTTON_CODE::kNumpad_3, "Keypad3)" },
			{ RE::BS_BUTTON_CODE::kNumpad_4, "Keypad4)" },
			{ RE::BS_BUTTON_CODE::kNumpad_5, "Keypad5)" },
			{ RE::BS_BUTTON_CODE::kNumpad_6, "Keypad6)" },
			{ RE::BS_BUTTON_CODE::kNumpad_7, "Keypad7)" },
			{ RE::BS_BUTTON_CODE::kNumpad_8, "Keypad8)" },
			{ RE::BS_BUTTON_CODE::kNumpad_9, "NumPad9)" },
			{ RE::BS_BUTTON_CODE::kNumpad_Period, "NumPadPeriod)" },
			{ RE::BS_BUTTON_CODE::kNumpad_Divide, "NumPadDivide)" },
			{ RE::BS_BUTTON_CODE::kNumpad_Multiply, "NumPadMult)" },
			{ RE::BS_BUTTON_CODE::kNumpad_Minus, "NumPadMinus)" },
			{ RE::BS_BUTTON_CODE::kNumpad_Plus, "NumPadPlus)" },
			//{ RE::BS_BUTTON_CODE::kNumpad_Enter, "KeypadEnter" }
		};

		Map<std::uint32_t, GamePadButton> gamePad{
			{ F4SE::InputMap::kGamepadButtonOffset_START, { "Start", "Options" } },
			{ F4SE::InputMap::kGamepadButtonOffset_BACK, { "Back", "Share" } },
			{ F4SE::InputMap::kGamepadButtonOffset_LEFT_THUMB, { "L3", "L3" } },
			{ F4SE::InputMap::kGamepadButtonOffset_RIGHT_THUMB, { "R3", "R3" } },
			{ F4SE::InputMap::kGamepadButtonOffset_LEFT_SHOULDER, { "LB", "L1" } },
			{ F4SE::InputMap::kGamepadButtonOffset_RIGHT_SHOULDER, { "RB", "R1" } },
			{ F4SE::InputMap::kGamepadButtonOffset_A, { "A", "Cross" } },
			{ F4SE::InputMap::kGamepadButtonOffset_B, { "B", "Circle" } },
			{ F4SE::InputMap::kGamepadButtonOffset_X, { "X", "Square" } },
			{ F4SE::InputMap::kGamepadButtonOffset_Y, { "Y", "Triangle" } },
			{ F4SE::InputMap::kGamepadButtonOffset_LT, { "LT", "L2" } },
			{ F4SE::InputMap::kGamepadButtonOffset_RT, { "RT", "R2" } },
		};

		Map<std::uint32_t, std::string> mouse{
			{ 256 + 0, "Mouse1)" },
			{ 256 + 1, "Mouse2)" },
			{ 256 + 2, "Mouse3)" },
			{ 256 + 3, "Mouse4)" },
			{ 256 + 4, "Mouse5)" },
			{ 256 + 5, "Mouse6)" },
			{ 256 + 6, "Mouse7)" },
			{ 256 + 7, "Mouse8)" },
		};

		BUTTON_SCHEME buttonScheme{ BUTTON_SCHEME::kAutoDetect };
	};
}

namespace ImGui
{
	ImVec2 ButtonIcon(std::uint32_t a_key);
	ImVec2 ButtonIcon(const std::string& a_key, bool a_useControllerFont, bool a_centerIcon);
	void   ButtonIconWithLabel(const char* a_text, const Font::Button& a_key, bool a_centerIcon);
}

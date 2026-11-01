#pragma once

#include "ImGui/Fonts/Manager.h"

namespace PhotoMode::Hotkeys
{
	class Manager : public REX::TSingleton<Manager>
	{
	public:
		void LoadHotKeys(const CSimpleIniA& a_ini);

		std::uint32_t        ResetKey() const;
		std::uint32_t        TakePhotoKey() const;
		std::uint32_t        ToggleMenusKey() const;
		std::uint32_t        NextTabKey() const;
		std::uint32_t        PreviousTabKey() const;
		std::uint32_t        FreezeTimeKey() const;
		static std::uint32_t EscapeKey();

		Font::Button ResetIcon() const;
		Font::Button TakePhotoIcon() const;
		Font::Button ToggleMenusIcon() const;
		Font::Button NextTabIcon() const;
		Font::Button PreviousTabIcon() const;
		Font::Button FreezeTimeIcon() const;

	private:
		struct Key
		{
			Key() = default;
			Key(RE::BS_BUTTON_CODE a_key, std::uint32_t a_gamePadKey) :
				keyboard(std::to_underlying(a_key)),
				gamePad(a_gamePadKey)
			{}

			void          LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting);
			std::uint32_t GetKey() const;

			std::uint32_t Keyboard() const;
			std::uint32_t GamePad() const;

		private:
			std::uint32_t keyboard{ 0 };
			std::uint32_t gamePad{ 0 };
		};

		Key nextTab{ RE::BS_BUTTON_CODE::kE, F4SE::InputMap::kGamepadButtonOffset_RT };   
		Key previousTab{ RE::BS_BUTTON_CODE::kQ, F4SE::InputMap::kGamepadButtonOffset_LT };
		Key takePhoto{ RE::BS_BUTTON_CODE::kSpace, F4SE::InputMap::kGamepadButtonOffset_BACK };
		Key toggleMenus{ RE::BS_BUTTON_CODE::kT, F4SE::InputMap::kGamepadButtonOffset_X };
		Key reset{ RE::BS_BUTTON_CODE::kR, F4SE::InputMap::kGamepadButtonOffset_Y };
		Key freezeTime{ RE::BS_BUTTON_CODE::kF, F4SE::InputMap::kGamepadButtonOffset_START };
	};
}

namespace Hotkeys = PhotoMode::Hotkeys;

#include "PhotoMode/Hotkeys.h"

#include "Input.h"
#include "PhotoMode/Manager.h"

namespace PhotoMode::Hotkeys
{
	void Manager::LoadHotKeys(const CSimpleIniA& a_ini)
	{
		reset.LoadKeys(a_ini, "iReset");
		takePhoto.LoadKeys(a_ini, "iTakePhoto");
		toggleMenus.LoadKeys(a_ini, "iToggleMenus");
		nextTab.LoadKeys(a_ini, "iNextTab");
		previousTab.LoadKeys(a_ini, "iPreviousTab");
		freezeTime.LoadKeys(a_ini, "iFreezeTime");
	}

	void Manager::Key::LoadKeys(const CSimpleIniA& a_ini, std::string_view a_setting)
	{
		keyboard = a_ini.GetLongValue("Controls", std::format("{}Key", a_setting).c_str(), keyboard);
		gamePad = a_ini.GetLongValue("Controls", std::format("{}GamePad", a_setting).c_str(), gamePad);
	}

	std::uint32_t Manager::Key::GetKey() const
	{
		auto device = MANAGER(Input)->GetInputDevice();
		return (device == Input::DEVICE::kKeyboard || device == Input::DEVICE::kMouse) ? keyboard : gamePad;
	}

	std::uint32_t Manager::Key::Keyboard() const
	{
		return keyboard;
	}

	std::uint32_t Manager::Key::GamePad() const
	{
		return gamePad;
	}

	std::uint32_t Manager::ResetKey() const
	{
		return reset.GetKey();
	}

	std::uint32_t Manager::TakePhotoKey() const
	{
		return takePhoto.GetKey();
	}

	std::uint32_t Manager::ToggleMenusKey() const
	{
		return toggleMenus.GetKey();
	}

	std::uint32_t Manager::NextTabKey() const
	{
		return nextTab.GetKey();
	}

	std::uint32_t Manager::PreviousTabKey() const
	{
		return previousTab.GetKey();
	}

	std::uint32_t Manager::FreezeTimeKey() const
	{
		return freezeTime.GetKey();
	}

	std::uint32_t Manager::EscapeKey()
	{
		auto device = MANAGER(Input)->GetInputDevice();
		return (device == Input::DEVICE::kKeyboard || device == Input::DEVICE::kMouse) ?
		           std::to_underlying(RE::BS_BUTTON_CODE::kEscape) :
		           F4SE::InputMap::kGamepadButtonOffset_B;
	}

	Font::Button Manager::ResetIcon() const
	{
		return MANAGER(Font)->GetIcon(reset.GetKey());
	}

	Font::Button Manager::TakePhotoIcon() const
	{
		return MANAGER(Font)->GetIcon(takePhoto.GetKey());
	}

	Font::Button Manager::ToggleMenusIcon() const
	{
		return MANAGER(Font)->GetIcon(toggleMenus.GetKey());
	}

	Font::Button Manager::NextTabIcon() const
	{
		return MANAGER(Font)->GetIcon(nextTab.GetKey());
	}

	Font::Button Manager::PreviousTabIcon() const
	{
		return MANAGER(Font)->GetIcon(previousTab.GetKey());
	}

	Font::Button Manager::FreezeTimeIcon() const
	{
		return MANAGER(Font)->GetIcon(freezeTime.GetKey());
	}
}

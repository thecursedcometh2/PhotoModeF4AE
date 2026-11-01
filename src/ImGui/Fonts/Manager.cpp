#include "ImGui/Fonts/Manager.h"

#include "ImGui/Fonts/IconsFontAwesome6.h"

#include "ImGui/Renderer.h"
#include "ImGui/Styles.h"
#include "ImGui/Util.h"
#include "Input.h"
#include "Settings.h"

namespace Font
{
	void Font::LoadSettings(const CSimpleIniA& a_ini, const char* a_section)
	{
		name = a_ini.GetValue(a_section, "sFont", name.c_str());
		name = R"(Data\Interface\ImGuiFonts\)" + name;

		const auto resolutionScale = ImGui::Renderer::GetResolutionScale();

		fontSize.first = static_cast<float>(a_ini.GetLongValue(a_section, "iSize", 32)) * resolutionScale;
		fontSize.second = static_cast<float>(a_ini.GetLongValue(a_section, "iLargeSize", 36)) * resolutionScale;

		iconSize.first = static_cast<float>(a_ini.GetLongValue("Icons", "iSize", 32)) * resolutionScale;
		iconSize.second = static_cast<float>(a_ini.GetLongValue("Icons", "iLargeSize", 36)) * resolutionScale;
	}

	void Font::LoadFont(bool a_loadIcons)
	{
		ImFontConfig config;

		const auto& io = ImGui::GetIO();
		config.Flags |= ImFontFlags_NoLoadError;
        font = io.Fonts->AddFontFromFileTTF(name.c_str(), 0.0f, &config);
        if (!font) {
            logger::error("Missing external ImGui Fonts asset: {}. Install Nexus mod 88107.", name);
            font = io.Fonts->AddFontDefault();
        }
		if (a_loadIcons) {
			config.MergeMode = true;
			config.PixelSnapH = true;
			config.OversampleH = config.OversampleV = 1;
			io.Fonts->AddFontFromFileTTF(R"(Data\Interface\ImGuiFonts\)" FONT_ICON_FILE_NAME_FAS, 0.0f, &config);
		}
	}

	ImFont* Font::GetFont() const
	{
		return font;
	}

	void Manager::LoadSettings()
	{
		Settings::GetSingleton()->Load(FileType::kFonts, [this](auto& ini) {
			defaultFont.LoadSettings(ini, "Font");
			controllerButtonFont.LoadSettings(ini, "ControllerButtonFont");
		});
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		buttonScheme = static_cast<BUTTON_SCHEME>(a_ini.GetLongValue("Controls", "iButtonScheme", std::to_underlying(buttonScheme)));
	}

	void Manager::ReloadFonts()
	{
		if (loadedFonts) {
			return;
		}

		auto& io = ImGui::GetIO();

		defaultFont.LoadFont(true);
		// No embedded Bethesda glyphs are distributed. Controller prompts use text labels.

		io.Fonts->Build();

		ImGui_ImplDX11_InvalidateDeviceObjects();
		ImGui_ImplDX11_CreateDeviceObjects();

		io.FontDefault = defaultFont.GetFont();

		loadedFonts = true;
	}

	std::pair<float, float> Manager::GetDefaultFontSize() const
	{
		return defaultFont.fontSize;
	}

	std::pair<float, float> Manager::GetDefaultIconSize() const
	{
		return defaultFont.iconSize;
	}

	ImFont* Manager::GetControllerButtonFont() const
	{
		return controllerButtonFont.GetFont();
	}

	Button Manager::GetIcon(std::uint32_t key)
	{
		if (auto inputDevice = MANAGER(Input)->GetInputDevice(); inputDevice == Input::DEVICE::kKeyboard || inputDevice == Input::DEVICE::kMouse) {
			if (key >= F4SE::InputMap::kMacro_MouseButtonOffset) {
				if (const auto it = mouse.find(key); it != mouse.end()) {
					return { it->second, false };
				}
			} else if (const auto it = keyboard.find(static_cast<RE::BS_BUTTON_CODE>(key)); it != keyboard.end()) {
				return { it->second, false };
			}
		} else {
			if (const auto it = gamePad.find(key); it != gamePad.end()) {
				return GetGamePadIcon(it->second);
			}
			return { unknownKeyController, false };
		}
		return { unknownKey, false };
	}

	Button Manager::GetGamePadIcon(const GamePadButton& a_icons) const
	{
		std::string text;

		switch (buttonScheme) {
		case BUTTON_SCHEME::kAutoDetect:
			text = MANAGER(Input)->GetInputDevice() == Input::DEVICE::kGamepadOrbis ? a_icons.ps4 : a_icons.xbox;
			break;
		case BUTTON_SCHEME::kXbox:
			text = a_icons.xbox;
			break;
		case BUTTON_SCHEME::kPS4:
			text = a_icons.ps4;
			break;
		default:
			text = a_icons.xbox;
			break;
		}

		return { text, false };
	}
}

ImVec2 ImGui::ButtonIcon(std::uint32_t a_key)
{
	const auto& [icon, useControllerFont] = MANAGER(Font)->GetIcon(a_key);
	return ButtonIcon(icon, useControllerFont, false);
}

ImVec2 ImGui::ButtonIcon(const std::string& a_key, bool a_useControllerFont, bool a_centerIcon)
{
	if (a_useControllerFont) {
		ImGui::PushFont(MANAGER(Font)->GetControllerButtonFont());
	}
	auto size = ImGui::CalcTextSize(a_key.c_str());
	if (a_centerIcon) {
		const float height = ImGui::GetWindowSize().y;
		ImGui::SetCursorPosY((height - size.y) / 2);
	}
	ImGui::TextUnformatted(a_key.c_str());
	if (a_useControllerFont) {
		ImGui::PopFont();
	}
	return size + GetStyle().ItemSpacing;
}

void ImGui::ButtonIconWithLabel(const char* a_text, const Font::Button& a_key, bool a_centerIcon)
{
	auto& [icon, useControllerButtonFont] = a_key;
	ImGui::ButtonIcon(icon, useControllerButtonFont, a_centerIcon);
	ImGui::SameLine();
	ImGui::TextUnformatted(a_text);
}

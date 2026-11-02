#pragma once
#include "WidgetID.h"

#include "Styles.h"
#include "Util.h"

#include "ImGui/Fonts/Manager.h"

namespace ImGui
{
	inline std::int32_t CURRENT_TAB{ -1 };
	inline float        MAX_WIDGET_PADDING{};

	void UpdateWidgetPadding(std::int32_t index, const char* paddingText);

	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items = -1);

	HoveredResult CheckBox(const char* label, bool* a_toggle);

	template <class E>
	HoveredResult EnumSlider(const char* label, E* index, const std::ranges::common_range auto& a_enum, bool a_translate = true)
	{
		const ScopedWidgetID widgetID(index);
		bool value_changed = false;

		std::size_t uIndex;
		if constexpr (std::is_enum_v<E>) {
			uIndex = std::to_underlying(*index);
		} else {
			uIndex = *index;
		}

		LeftAlignedText(label);

		const auto text = a_translate ? TRANSLATE_S(a_enum[uIndex]) : a_enum[uIndex];
		const auto frameHeight = ImGui::GetFrameHeightWithSpacing();

		SetCursorPosX(GetCursorPosX() + (MAX_WIDGET_PADDING - frameHeight));

		PushStyleColor(ImGuiCol_Button, ImVec4());
		PushStyleColor(ImGuiCol_ButtonActive, ImVec4());
		PushStyleColor(ImGuiCol_ButtonHovered, ImVec4());

		AlignForWidth(frameHeight + CalcTextSize(text.c_str()).x + frameHeight);

		const auto hovered = IsWidgetFocused(label);
		BeginGroup();
		{
			if (hovered) {
				PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
				PushStyleColor(ImGuiCol_TextShadow, GetColorU32(ImGuiCol_TextShadowDisabled));
			}
			ArrowButton(label, ImGuiDir_Left);
			SameLine();
			Text(text.c_str());
			SameLine();
			ArrowButton(label, ImGuiDir_Right);
			if (hovered) {
				PopStyleColor(2);
			}
		}
		EndGroup();

		PopStyleColor(3);

		if (hovered) {
			const bool pressedLeft = IsKeyPressed(ImGuiKey_LeftArrow) || IsKeyPressed(ImGuiKey_GamepadDpadLeft);
			const bool pressedRight = IsKeyPressed(ImGuiKey_RightArrow) || IsKeyPressed(ImGuiKey_Enter) || IsKeyPressed(ImGuiKey_GamepadDpadRight);
			if (pressedLeft) {
				uIndex = (uIndex - 1 + a_enum.size()) % a_enum.size();
			}
			if (pressedRight) {
				uIndex = (uIndex + 1) % a_enum.size();
			}
			if (pressedLeft || pressedRight) {
				value_changed = true;
				*index = static_cast<E>(uIndex);
				RE::UIUtils::PlayMenuSound("UIMenuPrevNext");
			}
		}

		if (IsItemFocused()) {
			UnfocusOnEscape();
		}

		return { value_changed, hovered };
	}

	HoveredResult ThinSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags);
	HoveredResult ThinSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags);
	HoveredResult ThinSliderUInt(const char* label, std::uint32_t* v, std::uint32_t v_min, std::uint32_t v_max, const char* format, ImGuiSliderFlags flags);

	template <class T>
	HoveredResult Slider(const char* label, T* v, T v_min, T v_max, const char* format = nullptr, ImGuiSliderFlags flags = ImGuiSliderFlags_AlwaysClamp)
	{
		const ScopedWidgetID widgetID(v);
		LeftAlignedText(label);

		PushStyleColor(ImGuiCol_FrameBg, GetUserStyleColorU32(USER_STYLE::kFrameBG_Widget));
		PushStyleColor(ImGuiCol_FrameBgHovered, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));
		PushStyleColor(ImGuiCol_FrameBgActive, GetUserStyleColorU32(USER_STYLE::kFrameBG_WidgetActive));

		HoveredResult result;
		if constexpr (std::is_floating_point_v<T>) {
			result = ImGui::ThinSliderFloat(label, v, v_min, v_max, format ? format : "%.2f", flags | ImGuiSliderFlags_NoInput);
		} else if constexpr (std::is_signed_v<T>) {
			result = ImGui::ThinSliderInt(label, v, v_min, v_max, format ? format : "%d", flags | ImGuiSliderFlags_NoInput);
		} else {
			result = ImGui::ThinSliderUInt(label, v, v_min, v_max, format ? format : "%d", flags | ImGuiSliderFlags_NoInput);
		}
		if (result.first) {
			RE::UIUtils::PlayMenuSound("UIMenuPrevNext");
		}
		ActivateOnHover();

		PopStyleColor(3);

		return result;
	}

	bool OpenTabOnHover(const char* a_label, ImGuiTabItemFlags flags = 0);
}

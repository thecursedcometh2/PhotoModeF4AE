#include "ImGui/Widgets.h"

namespace ImGui
{
	// Source: https://gist.github.com/idbrii/5ddb2135ca122a0ec240ce046d9e6030
	//
	// Author: David Briscoe
	//
	// Modified ComboWithFilter with rapidFuzz
	// Using dear imgui, v1.89 WIP
	//
	// Adds arrow/pgup/pgdn navigation, Enter to confirm, max_height_in_items, and
	// fixed focus on open and avoids drawing past window edges.
	//
	// Licensed as CC0/public domain.
	//
	// Posted in issue: https://github.com/ocornut/imgui/issues/1658#issuecomment-1086193100

	void UpdateWidgetPadding(std::int32_t index, const char* paddingText)
	{
		if (CURRENT_TAB != index) {
			CURRENT_TAB = index;
			MAX_WIDGET_PADDING = ImGui::CalcTextSize(paddingText).x;
		}
	}

	bool ComboWithFilter(const char* label, int* current_item, const std::vector<std::string>& items, int popup_max_height_in_items /*= -1*/)
	{
		const ScopedWidgetID widgetID(current_item);
		const bool allow_repeat = ImGuiInputFlags_Repeat;

		ImGuiContext& g = *GImGui;

		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return false;

		int items_count = static_cast<int>(items.size());

		// Could use imgui Items_ getters to support more input formats.
		const char* preview_value = NULL;
		if (*current_item >= 0 && *current_item < items_count)
			preview_value = items[*current_item].c_str();

		static int  focus_idx = -1;
		static char pattern_buffer[256] = { 0 };

		bool value_changed = false;

		const ImGuiID id = window->GetID(label);
		const ImGuiID popup_id = ImHashStr("##ComboPopup", 0, id);  // copied from BeginCombo
		const bool    is_already_open = IsPopupOpen(popup_id, ImGuiPopupFlags_None);
		const bool    is_filtering = is_already_open && pattern_buffer[0] != '\0';

		int show_count = items_count;

		std::vector<std::pair<int, double>> itemScoreVector;
		if (is_filtering) {
			// Filter before opening to ensure we show the correct size window.
			// We won't get in here unless the popup is open.
			for (int i = 0; i < items_count; i++) {
				auto score = rapidfuzz::fuzz::partial_token_ratio(pattern_buffer, items[i].c_str());
				if (score >= 65.0) {
					itemScoreVector.push_back(std::make_pair(i, score));
				}
			}
			std::ranges::sort(itemScoreVector, [](const auto& a, const auto& b) {
				return (b.second < a.second);
			});
			const int current_score_idx = IndexOfKey(itemScoreVector, focus_idx);
			if (current_score_idx < 0 && !itemScoreVector.empty()) {
				focus_idx = itemScoreVector[0].first;
			}
			show_count = static_cast<int>(itemScoreVector.size());
		}

		// Define the height to ensure our size calculation is valid.
		if (popup_max_height_in_items == -1) {
			popup_max_height_in_items = 5;
		}
		popup_max_height_in_items = ImMin(popup_max_height_in_items, show_count);

		if (!(g.NextWindowData.HasFlags & ImGuiNextWindowDataFlags_HasSizeConstraint)) {
			int numItems = popup_max_height_in_items + 3;  // extra for search bar
			SetNextWindowSizeConstraints(ImVec2(0, 0), ImVec2(FLT_MAX, CalcMaxPopupHeightFromItemCount(numItems)));
		}

		if (!BeginCombo(label, preview_value, ImGuiComboFlags_None))
			return false;

		if (!is_already_open) {
			focus_idx = *current_item;
			memset(pattern_buffer, 0, IM_ARRAYSIZE(pattern_buffer));
		}

		ImGui::PushStyleColor(ImGuiCol_FrameBg, GetUserStyleColorVec4(USER_STYLE::kComboBoxTextBox));
		ImGui::PushStyleColor(ImGuiCol_Text, GetUserStyleColorVec4(USER_STYLE::kComboBoxText));
		ImGui::PushStyleColor(ImGuiCol_NavCursor, ImVec4());
		ImGui::PushItemWidth(-FLT_MIN);
		// Filter input
		if (!is_already_open)
			ImGui::SetKeyboardFocusHere();
		InputText("##ComboWithFilter_inputText", pattern_buffer, 256, ImGuiInputTextFlags_AutoSelectAll);

		ImGui::PopStyleColor(3);

		int move_delta = 0;
		// Use Shortcut to prevent NavEnableKeyboard from also responding to nav.
		if (Shortcut(ImGuiKey_UpArrow, allow_repeat) || IsKeyPressed(ImGuiKey_GamepadDpadUp)) {
			--move_delta;
		} else if (Shortcut(ImGuiKey_DownArrow, allow_repeat) || IsKeyPressed(ImGuiKey_GamepadDpadDown)) {
			++move_delta;
		} else if (Shortcut(ImGuiKey_PageUp, ImGuiInputFlags_None)) {
			move_delta -= popup_max_height_in_items;
		} else if (Shortcut(ImGuiKey_PageDown, ImGuiInputFlags_None)) {
			move_delta += popup_max_height_in_items;
		}

		if (move_delta != 0) {
			if (is_filtering) {
				int current_score_idx = IndexOfKey(itemScoreVector, focus_idx);
				if (current_score_idx >= 0) {
					const int count = static_cast<int>(itemScoreVector.size());
					current_score_idx = ImClamp(current_score_idx + move_delta, 0, count - 1);
					focus_idx = itemScoreVector[current_score_idx].first;
				}
			} else {
				focus_idx = ImClamp(focus_idx + move_delta, 0, items_count - 1);
			}
			RE::UIUtils::PlayMenuSound("UIMenuPrevNext");
		}

		// Copied from ListBoxHeader
		// If popup_max_height_in_items == -1, default height is maximum 7.
		float  height_in_items_f = (popup_max_height_in_items < 0 ? ImMin(items_count, 7) : popup_max_height_in_items) + 0.25f;
		ImVec2 size;
		size.x = 0.0f;
		size.y = GetTextLineHeightWithSpacing() * height_in_items_f + g.Style.FramePadding.y * 2.0f;

		ImGui::PushStyleColor(ImGuiCol_NavCursor, ImVec4());
		if (ImGui::BeginListBox("##ComboWithFilter_itemList", size)) {
			for (int i = 0; i < show_count; i++) {
				int idx = is_filtering ? itemScoreVector[i].first : i;
				PushID(reinterpret_cast<void*>(static_cast<intptr_t>(idx)));
				const bool  item_selected = (idx == focus_idx);
				const char* item_text = items[idx].c_str();
				if (item_selected) {
					PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
					PushStyleColor(ImGuiCol_TextShadow, GetColorU32(ImGuiCol_TextShadowDisabled));
				}
				if (Selectable(item_text, item_selected)) {
					value_changed = true;
					*current_item = idx;
					CloseCurrentPopup();
					RE::UIUtils::PlayMenuSound("UIMenuFocus");
				}
				if (item_selected) {
					SetItemDefaultFocus();
					// SetItemDefaultFocus doesn't work so also check IsWindowAppearing.
					if (move_delta != 0 || IsWindowAppearing()) {
						SetScrollHereY();
					}
					PopStyleColor(2);
				}
				PopID();
			}
			ImGui::EndListBox();

			const bool repeat = false;
			if (IsKeyPressed(ImGuiKey_Enter, repeat) || IsKeyPressed(ImGuiKey_Space, repeat) || IsKeyPressed(ImGuiKey_NavGamepadActivate)) {
				value_changed = true;
				*current_item = focus_idx;
				CloseCurrentPopup();
				RE::UIUtils::PlayMenuSound("UIMenuOK");
			}
			// Using Shortcut would make the first Esc clear input (even if empty) and the second close the popup.
			else if (IsKeyPressed(ImGuiKey_Escape, repeat) || IsKeyPressed(ImGuiKey_NavGamepadCancel)) {
				value_changed = false;
				CloseCurrentPopup();
				RE::UIUtils::PlayMenuSound("UIMenuCancel");
			}
		}
		ImGui::PopStyleColor();
		ImGui::PopItemWidth();
		ImGui::EndCombo();

		if (value_changed)
			MarkItemEdited(g.LastItemData.ID);

		return value_changed;
	}

	HoveredResult CheckBox(const char* label, bool* a_toggle)
	{
        const ScopedWidgetID widgetID(a_toggle);
		bool selected = false;

		LeftAlignedText(label);

		float spacing = ImGui::GetStyle().ItemSpacing.x;
		SetCursorPosX(GetCursorPosX() + (MAX_WIDGET_PADDING - spacing));

		PushFont(NULL, MANAGER(Font)->GetDefaultFontSize().second);

		auto toggleText = *a_toggle ? TRANSLATE("$ON") : TRANSLATE("$OFF");
		AlignForWidth(CalcTextSize(toggleText).x + spacing);

		const auto hovered = IsWidgetFocused(label);
		BeginGroup();
		{
			InvisibleButton(label, ImVec2(0.1f, 0.1f), ImGuiButtonFlags_EnableNav);
			SameLine();
			if (hovered) {
				PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
				PushStyleColor(ImGuiCol_TextShadow, GetColorU32(ImGuiCol_TextShadowDisabled));
			}
			TextUnformatted(toggleText);
			if (hovered) {
				PopStyleColor(2);
			}
		}
		EndGroup();

		PopFont();

		if (IsItemFocused()) {
			if (IsKeyPressed(ImGuiKey_Enter) || IsKeyPressed(ImGuiKey_GamepadFaceDown)) {
				*a_toggle = !*a_toggle;
				selected = true;
			} else {
				UnfocusOnEscape();
			}
		}

		if (selected) {
			RE::UIUtils::PlayMenuSound("UIMenuFocus");
		}
		return { selected, hovered };
	}

	void SliderValue(const char* label, ImGuiDataType data_type, void* p_data, const char* format)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return;

		ImGuiContext&     g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID     id = window->GetID(label);
		const bool        isHovered = GetFocusID() == id;
		ImVec2            cursorPos = GetCursorPos();

		// Default format string when passing NULL
		if (format == nullptr)
			format = DataTypeGetInfo(data_type)->PrintFmt;

		// Display value using user-provided display format so user can add prefix/suffix/decorations to the value.
		char        value_buf[64];
		const char* value_buf_end = value_buf + DataTypeFormatString(value_buf, IM_ARRAYSIZE(value_buf), data_type, p_data, format);

		AlignTextToFramePadding();

		auto width = CalcTextSize(value_buf, value_buf_end).x;

		if (width < MAX_WIDGET_PADDING) {
			SetCursorPosX(cursorPos.x + (MAX_WIDGET_PADDING - width));
		}

		if (isHovered) {
			PushStyleColor(ImGuiCol_Text, GetColorU32(ImGuiCol_TextDisabled));
			PushStyleColor(ImGuiCol_TextShadow, GetColorU32(ImGuiCol_TextShadowDisabled));
		}
		TextUnformatted(value_buf, value_buf_end);
		if (isHovered) {
			PopStyleColor(2);
		}

		SameLine(0, style.ItemInnerSpacing.x);
		SetCursorPosX(cursorPos.x + MAX_WIDGET_PADDING + style.ItemSpacing.x);
		SetNextItemWidth(GetContentRegionAvail().x - style.ItemSpacing.x);
	}

	HoveredResult ThinSliderScalar(const char* label, ImGuiDataType data_type, void* p_data, const void* p_min, const void* p_max, const char* format, ImGuiSliderFlags flags, float sliderThickness)
	{
		ImGuiWindow* window = GetCurrentWindow();
		if (window->SkipItems)
			return { false, false };

		ImGuiContext&     g = *GImGui;
		const ImGuiStyle& style = g.Style;
		const ImGuiID     id = window->GetID(label);
		const bool        isHovered = GetFocusID() == id;
		const float       w = CalcItemWidth();

		const ImVec2 label_size = CalcTextSize(label, nullptr, true);
		const ImRect frame_bb(window->DC.CursorPos, window->DC.CursorPos + ImVec2(w, label_size.y + style.FramePadding.y * 2.0f));
		const ImRect total_bb(frame_bb.Min, frame_bb.Max + ImVec2(label_size.x > 0.0f ? style.ItemInnerSpacing.x + label_size.x : 0.0f, 0.0f));

		const bool temp_input_allowed = (flags & ImGuiSliderFlags_NoInput) == 0;
		ItemSize(total_bb, style.FramePadding.y);
		if (!ItemAdd(total_bb, id, &frame_bb, temp_input_allowed ? ImGuiItemFlags_Inputable : 0))
			return { false, false };

		// Default format string when passing NULL
		if (format == nullptr)
			format = DataTypeGetInfo(data_type)->PrintFmt;

		const bool hovered = ItemHoverable(frame_bb, id, g.LastItemData.ItemFlags);
		bool       temp_input_is_active = temp_input_allowed && TempInputIsActive(id);
		if (!temp_input_is_active) {
			// Tabbing or CTRL-clicking on Slider turns it into an input box
			const bool clicked = hovered && IsMouseClicked(0, ImGuiInputFlags_None, id);
			const bool make_active = (clicked || g.NavActivateId == id);
			if (make_active && clicked)
				SetKeyOwner(ImGuiKey_MouseLeft, id);
			if (make_active && temp_input_allowed)
				if ((clicked && g.IO.KeyCtrl) || (g.NavActivateId == id && (g.NavActivateFlags & ImGuiActivateFlags_PreferInput)))
					temp_input_is_active = true;

			// Store initial value (not used by main lib but available as a convenience but some mods e.g. to revert)
			if (make_active)
				memcpy(&g.ActiveIdValueOnActivation, p_data, DataTypeGetInfo(data_type)->Size);

			if (make_active && !temp_input_is_active) {
				SetActiveID(id, window);
				SetFocusID(id, window);
				FocusWindow(window);
				g.ActiveIdUsingNavDirMask |= (1 << ImGuiDir_Left) | (1 << ImGuiDir_Right);
			}
		}

		if (temp_input_is_active) {
			// Only clamp CTRL+Click input when ImGuiSliderFlags_AlwaysClamp is set
			const bool is_clamp_input = (flags & ImGuiSliderFlags_AlwaysClamp) != 0;
			return { TempInputScalar(frame_bb, id, label, data_type, p_data, format, is_clamp_input ? p_min : nullptr, is_clamp_input ? p_max : nullptr), isHovered };
		}

		// Draw frame
		const ImU32 frame_col = GetColorU32(g.ActiveId == id ? ImGuiCol_FrameBgActive : hovered ? ImGuiCol_FrameBgHovered :
																								  ImGuiCol_FrameBg);
		// RenderNavHighlight(frame_bb, id);
		// RenderFrame(frame_bb.Min, frame_bb.Max, frame_col, true, g.Style.FrameRounding);

		// Slider behavior
		ImRect     grab_bb;
		const bool value_changed = SliderBehavior(frame_bb, id, data_type, p_data, p_min, p_max, format, flags, &grab_bb);
		if (value_changed)
			MarkItemEdited(id);

		ImRect     draw_bb = frame_bb;
		const auto shrink_amount = static_cast<float>(static_cast<int>((frame_bb.Max.y - frame_bb.Min.y) * 0.5f * (1.0f - sliderThickness)));
		draw_bb.Min.y += shrink_amount;
		draw_bb.Max.y -= shrink_amount;

		// Render track
		window->DrawList->AddRect(draw_bb.Min, draw_bb.Max, isHovered ? GetUserStyleColorU32(USER_STYLE::kSliderBorderActive) : GetUserStyleColorU32(USER_STYLE::kSliderBorder), g.Style.FrameRounding, 0, 3.0f);

		window->DrawList->AddRectFilled(draw_bb.Min, ImVec2(grab_bb.Min.x + (grab_bb.Max.x - grab_bb.Min.x) * 0.5f, draw_bb.Max.y), frame_col, style.FrameRounding, ImDrawFlags_RoundCornersLeft);
		window->DrawList->AddRectFilled(ImVec2(grab_bb.Max.x - (grab_bb.Max.x - grab_bb.Min.x) * 0.5f, draw_bb.Min.y), draw_bb.Max, frame_col, style.FrameRounding, ImDrawFlags_RoundCornersRight);

		// Render grab
		if (grab_bb.Max.x > grab_bb.Min.x)
			window->DrawList->AddRectFilled(grab_bb.Min, grab_bb.Max, GetColorU32(g.ActiveId == id ? ImGuiCol_SliderGrabActive : ImGuiCol_SliderGrab), style.GrabRounding);

		return { value_changed, isHovered };
	}

	HoveredResult ThinSliderFloat(const char* label, float* v, float v_min, float v_max, const char* format, ImGuiSliderFlags flags)
	{
		SliderValue(label, ImGuiDataType_Float, v, format);
		return ThinSliderScalar(label, ImGuiDataType_Float, v, &v_min, &v_max, format, flags, 0.5f);
	}

	HoveredResult ThinSliderInt(const char* label, int* v, int v_min, int v_max, const char* format, ImGuiSliderFlags flags)
	{
		SliderValue(label, ImGuiDataType_S32, v, format);
		return ThinSliderScalar(label, ImGuiDataType_S32, v, &v_min, &v_max, format, flags, 0.5f);
	}

	HoveredResult ThinSliderUInt(const char* label, std::uint32_t* v, std::uint32_t v_min, std::uint32_t v_max, const char* format, ImGuiSliderFlags flags)
	{
		SliderValue(label, ImGuiDataType_U32, v, format);
		return ThinSliderScalar(label, ImGuiDataType_U32, v, &v_min, &v_max, format, flags, 0.5f);
	}

	bool BeginTabItemEx(const char* label, ImGuiID* active_tab, bool* p_open = nullptr, ImGuiTabItemFlags flags = 0)
	{
		ImGuiID id = GetID(label);
		bool    wasActive = *active_tab == id;

		if (wasActive) {
			PushStyleColor(ImGuiCol_Text, GetStyleColorVec4(ImGuiCol_TextDisabled));
			PushStyleColor(ImGuiCol_TextShadow, GetColorU32(ImGuiCol_TextShadowDisabled));
		}

		const bool isActive = BeginTabItem(label, p_open, flags);

		if (wasActive)
			PopStyleColor(2);

		if (isActive)
			*active_tab = id;

		return isActive;
	}

	bool OpenTabOnHover(const char* a_label, const ImGuiTabItemFlags flags)
	{
		static ImGuiID activeInspectorTab;
		const bool     selected = BeginTabItemEx(a_label, &activeInspectorTab, nullptr, flags);
		if (!selected && ActivateOnHover()) {
			RE::UIUtils::PlayMenuSound("UIJournalTabsSD");
		}
		return selected;
	}
}

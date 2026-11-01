#pragma once

namespace ImGui
{
	enum class USER_STYLE
	{
		kIcon,
		kIconDisabled,
		kButtons,
		kGridLines,
		kSliderBorder,
		kSliderBorderActive,
		kFrameBG_Widget,
		kFrameBG_WidgetActive,
		kComboBoxTextBox,
		kComboBoxText,
		kSelectedText,
		kSelectedBG,
	};

	class Styles : public REX::TSingleton<Styles>
	{
	public:
		ImU32  GetColorU32(USER_STYLE a_style) const;
		ImVec4 GetColorVec4(USER_STYLE a_style) const;
		float  GetVar(USER_STYLE a_style) const;

		void OnStyleRefresh();

	private:
		ImVec4 GetColor(const RE::NiColor& a_color);
		void   SetColor(ImVec4& a_to, const ImVec4& a_from);

		struct Style
		{
			// unused, helpers
			float bgAlpha{ 0.68f };
			float disabledAlpha{ 0.35f };

			ImVec4 icon{ 1.0f, 1.0f, 1.0f, 1.0f };
			ImVec4 iconDisabled{ 1.0f, 1.0f, 1.0f, 1.0f };

			ImVec4 background{ 0.0f, 0.0f, 0.0f, bgAlpha };

			ImVec4 border{ 0.569f, 0.545f, 0.506f, bgAlpha };
			float  borderSize{ 3.5f };

			ImVec4 text{ 1.0f, 1.0f, 1.0f, 1.0f };
			ImVec4 textDisabled{ 1.0f, 1.0f, 1.0f, 1.0f };
			ImVec4 textShadowDisabled{ 0.0f, 0.0f, 0.0f, 0.0f };

			ImVec4 comboBoxText{ 1.0f, 1.0f, 1.0f, 0.8f };     //  searchbox text
			ImVec4 comboBoxTextBox{ 0.0f, 0.0f, 0.0f, 1.0f };  // searchbox bg

			ImVec4 button{ 0.0f, 0.0f, 0.0f, bgAlpha };  // arrow button

			ImVec4 frameBG{ 0.2f, 0.2f, 0.2f, bgAlpha };
			ImVec4 frameBG_Widget{ 1.0f, 1.0f, 1.0f, 0.06275f };    //sliders
			ImVec4 frameBG_WidgetActive{ 1.0f, 1.0f, 1.0f, 0.2f };  //sliders

			ImVec4 sliderGrab{ 1.0f, 1.0f, 1.0f, 0.245f };
			ImVec4 sliderGrabActive{ 1.0f, 1.0f, 1.0f, 0.531f };

			ImVec4 header{ 1.0f, 1.0f, 1.0f, 0.1f };  // select highlight
			ImVec4 tab{ 0.0f, 0.0f, 0.0f, 0.0f };
			ImVec4 tabHovered{ 0.2f, 0.2f, 0.2f, 1.0f };

			ImVec4 gridLines{ 1.0f, 1.0f, 1.0f, 0.3333f };
			float  gridThickness{ 2.5f };

			ImVec4 separator{ 0.569f, 0.545f, 0.506f, bgAlpha };

			ImVec4 sliderBorder{ 1.0f, 1.0f, 1.0f, 0.2431f };
			ImVec4 sliderBorderActive{ 1.0f, 1.0f, 1.0f, 0.8f };
		} user;

		// for values that require ImU32 colours
		// can't read directly, repeated (ARGB -> RGBA -> ARGB) conversion is broken
		ImU32 frameBG_WidgetU32;
		ImU32 frameBG_WidgetActiveU32;
		ImU32 gridLinesU32;
		ImU32 sliderBorderU32;
		ImU32 sliderBorderActiveU32;
		ImU32 iconU32;
		ImU32 iconDisabledU32;
		ImU32 selectedBGU32;
		ImU32 selectedTextU32;

		bool   refreshStyle{ true };
		ImVec4 cachedHUDColor{};
		ImVec4 cachedBGColor{};
	};

	ImU32  GetUserStyleColorU32(USER_STYLE a_style);
	ImVec4 GetUserStyleColorVec4(USER_STYLE a_style);
	float  GetUserStyleVar(USER_STYLE a_style);
}

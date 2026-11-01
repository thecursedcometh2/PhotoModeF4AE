#include "PhotoMode/Tabs/Filters.h"

namespace PhotoMode
{
	void Filters::GetOriginalState()
	{
		imods.InitForms();

		const auto IMGS = RE::ImageSpaceManager::GetSingleton();
		originalOverride = IMGS->overRideBaseData;
        if (const auto effective = originalOverride ? originalOverride : IMGS->baseData) {
            imageSpaceData = *effective;
        }
		IMGS->overRideBaseData = &imageSpaceData;
	}

	void Filters::RevertState(bool a_fullReset)
	{
		// reset imagespace
		const auto IMGS = RE::ImageSpaceManager::GetSingleton();
		if (a_fullReset) {
			if (IMGS->overRideBaseData == &imageSpaceData) {
                IMGS->overRideBaseData = originalOverride;
            }
            originalOverride = nullptr;
		} else if (IMGS->overRideBaseData) {
			if (IMGS->baseData) {
				imageSpaceData = *IMGS->baseData;
			}
			IMGS->overRideBaseData = &imageSpaceData;
		}

		// reset imod
		imods.Reset();
		if (imodPlayed) {
			if (currentImod) {
				RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
				currentImod = nullptr;
			}
			imodPlayed = false;
		}
	}

	void Filters::Draw()
	{
		ImGui::UpdateWidgetPadding(4, "3.00");

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSplit(2);

		if (auto overrideData = const_cast<RE::ImageSpaceBaseData*>(RE::ImageSpaceManager::GetSingleton()->overRideBaseData)) {
			ImGui::DisplayWidgetsGroups([&] {
				return ImGui::Slider("$PM_Brightness"_T, &overrideData->cinematicData.brightness, 0.0f, 3.0f).second;
			});
			ImGui::DisplayWidgetsGroups([&] {
				return ImGui::Slider("$PM_Saturation"_T, &overrideData->cinematicData.saturation, 0.0f, 3.0f).second;
			});
			ImGui::DisplayWidgetsGroups([&] {
				return ImGui::Slider("$PM_Contrast"_T, &overrideData->cinematicData.contrast, 0.0f, 3.0f).second;
			});
			ImGui::DisplayWidgetsGroups([&] {
				return ImGui::Slider("$PM_TintAlpha"_T, &overrideData->tintData.amount, 0.0f, 1.0f).second;
			});
			ImGui::Indent();
			{
				ImGui::DisplayWidgetsGroups([&] {
					return ImGui::Slider("$PM_TintRed"_T, &overrideData->tintData.color.red, 0.0f, 1.0f).second;
				});
				ImGui::DisplayWidgetsGroups([&] {
					return ImGui::Slider("$PM_TintBlue"_T, &overrideData->tintData.color.blue, 0.0f, 1.0f).second;
				});
				ImGui::DisplayWidgetsGroups([&] {
					return ImGui::Slider("$PM_TintGreen"_T, &overrideData->tintData.color.green, 0.0f, 1.0f).second;
				});
			}
			ImGui::Unindent();
		} else {
			RE::ImageSpaceManager::GetSingleton()->overRideBaseData = &imageSpaceData;
		}

		imods.GetFormResultFromCombo([&](const auto& imod) {
			if (currentImod) {
				RE::ImageSpaceModifierInstanceForm::Stop(currentImod);
			}
			RE::ImageSpaceModifierInstanceForm::Trigger(imod, 1.0, nullptr);
			currentImod = imod;
			imodPlayed = true;
		});

		drawList->ChannelsMerge();
	}
}

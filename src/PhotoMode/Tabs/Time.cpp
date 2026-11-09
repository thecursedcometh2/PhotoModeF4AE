#include "PhotoMode/Tabs/Time.h"

#include "ImGui/Widgets.h"

namespace PhotoMode
{
	void Time::OriginalState::Get()
	{
		freezeTime = RE::Main::GetSingleton()->freezeTime;
		globalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();

		const auto calendar = RE::Calendar::GetSingleton();
		timescale = calendar->timeScale->value;
		gameHour = calendar->gameHour->value;
	}

	void Time::OriginalState::Revert() const
	{
		RE::Main::GetSingleton()->freezeTime = freezeTime;
		RE::BSTimer::GetSingleton()->SetGlobalTimeMultiplier(globalTimeMult, true);

		const auto calendar = RE::Calendar::GetSingleton();
		calendar->timeScale->value = timescale;
		calendar->gameHour->value = gameHour;
	}

	void Time::GetOriginalState()
	{
		originalState.Get();
		weathers.InitForms();
	}

	void Time::RevertState()
	{
		originalState.Revert();

		currentTimescaleMult = 1.0f;
		currentGlobalTimeMult = 1.0f;

		// revert weather
		weathers.Reset();
		if (weatherForced) {
			const auto sky = RE::Sky::GetSingleton();
			sky->ReleaseWeatherOverride();
			sky->ResetWeather();
			if (originalWeather) {
				sky->ForceWeather(originalWeather, false);
			}
			weatherForced = false;
		}
	}

	void Time::OnFrameUpdate() const
	{
		if (weatherForced) {
			RE::Sky::GetSingleton()->lastWeatherUpdate = RE::Calendar::GetSingleton()->gameHour->value;
		}
	}

	void Time::Draw()
	{
		ImGui::UpdateWidgetPadding(1, "23:99 PM");

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSplit(2);

		ImGui::DisplayWidgetsGroups([&] {
			return ImGui::CheckBox("$PM_FreezeTime"_T, &RE::Main::GetSingleton()->freezeTime).second;
		});

		ImGui::DisplayWidgetsGroups([&] {
			currentGlobalTimeMult = RE::BSTimer::QGlobalTimeMultiplier();
			auto [selected, hovered] = ImGui::Slider("$PM_GlobalTimeMult"_T, &currentGlobalTimeMult, 0.01f, 2.0f);
			if (selected) {
				RE::BSTimer::GetSingleton()->SetGlobalTimeMultiplier(currentGlobalTimeMult, true);
			}
			return hovered;
		});

		ImGui::DisplayWidgetsGroups([&] {
			auto& gameHour = RE::Calendar::GetSingleton()->gameHour->value;
			return ImGui::Slider("$PM_GameHour"_T, &gameHour, 0.0f, 23.99f, std::format("{:%I:%M %p}", std::chrono::duration<float, std::ratio<3600>>(gameHour)).c_str()).second;
		});

		ImGui::DisplayWidgetsGroups([&] {
			auto [selected, hovered] = ImGui::Slider("$PM_TimeScaleMult"_T, &currentTimescaleMult, 1.0f, 1000.0f, "%.fX");
			if (selected) {
				RE::Calendar::GetSingleton()->timeScale->value = originalState.timescale * currentTimescaleMult;
			}
			return hovered;
		});

		auto isSkyInterior = RE::Sky::GetSingleton()->mode == RE::Sky::Mode::kInterior;
		if (isSkyInterior) {
			ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ImGui::GetStyle().DisabledAlpha);
		}
		ImGui::BeginDisabled(isSkyInterior);
		{
			weathers.GetFormResultFromCombo([&](const auto& weather) {
				RE::Sky::GetSingleton()->ForceWeather(weather, true);
				weatherForced = true;
			});
		}
		ImGui::EndDisabled();
		if (isSkyInterior) {
			ImGui::PopStyleVar();
		}

		drawList->ChannelsMerge();
	}
}

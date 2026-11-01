#include "PhotoMode/Tabs/Camera.h"

#include "ImGui/Widgets.h"

namespace PhotoMode
{
	void SetFOV(float a_fov)
	{
		RE::Main::GetSingleton()->SetCameraFOV(a_fov);
		RE::PlayerCamera::GetSingleton()->worldFOV = a_fov;
	}

	void Camera::OriginalState::Get()
	{
		fov = RE::PlayerCamera::GetSingleton()->worldFOV;
		translateSpeed = RE::GetINISetting("fFreeCameraTranslationSpeed:Camera")->GetFloat();
	}

	void Camera::OriginalState::Revert([[maybe_unused]] bool a_deactivate) const
	{
		SetFOV(fov);
	}

	void Camera::GetOriginalState()
	{
		originalState.Get();

		if (!fFreeCameraTranslationSpeed) {
			fFreeCameraTranslationSpeed = RE::INISettingCollection::GetSingleton()->GetSetting("fFreeCameraTranslationSpeed:Camera");
		}

		if (!dofInstance) {
			dofInstance.reset(RE::ImageSpaceModifierInstanceDOF::Trigger(
				originalState.defaultDOF.distance,
				originalState.defaultDOF.range,
				0.0f,
				0.0f,
				RE::ImageSpaceModifierInstanceDOF::DepthOfFieldMode::kBack,
				originalState.defaultDOF.strength,
				0.0f));
			dofInstance->flags.set(RE::ImageSpaceModifierInstance::FLAGS::kPermanent);
		}

		enableDOF = false;
	}

	void Camera::RevertState(bool a_deactivate)
	{
		originalState.Revert(a_deactivate);

		// revert view roll
		currentViewRoll = 0.0f;

		// revert translation speed
		if (a_deactivate) {
			if (fFreeCameraTranslationSpeed) {
				fFreeCameraTranslationSpeed->SetFloat(originalState.translateSpeed);
				fFreeCameraTranslationSpeed = nullptr;
			}
		}

		// revert grid
		CameraGrid::gridType = CameraGrid::GridType::kDisabled;

		// revert DOF
		enableDOF = false;
		if (a_deactivate) {
			if (dofInstance) {
				dofInstance->Stop();
				dofInstance.reset();
			}
		} else {
			if (dofInstance) {
				dofInstance->strength = originalState.defaultDOF.strength;
				dofInstance->data.depthOfField.distance = originalState.defaultDOF.distance;
				dofInstance->data.depthOfField.range = originalState.defaultDOF.range;
			}
		}
	}

	void Camera::Draw()
	{
		ImGui::UpdateWidgetPadding(0, "5000.00");

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSplit(2);

		ImGui::DisplayWidgetsGroups([] {
			return ImGui::EnumSlider("$PM_Grid"_T, &CameraGrid::gridType, CameraGrid::gridTypes).second;
		});

		ImGui::DisplayWidgetsGroups([&] {
			currentFOV = RE::PlayerCamera::GetSingleton()->worldFOV;
			auto [selected, hovered] = ImGui::Slider("$PM_FieldOfView"_T, &currentFOV, 5.0f, 150.0f);
			if (selected) {
				SetFOV(currentFOV);
			}
			return hovered;
		});

		ImGui::DisplayWidgetsGroups([&] {
			currentViewRollDegrees = RE::rad_to_deg(currentViewRoll);
			auto [selected, hovered] = ImGui::Slider("$PM_ViewRoll"_T, &currentViewRollDegrees, -90.0f, 90.0f);
			if (selected) {
				currentViewRoll = RE::deg_to_rad(currentViewRollDegrees);
			}
			return hovered;
		});

		if (fFreeCameraTranslationSpeed) {
			ImGui::DisplayWidgetsGroups([&] {
				currentTranslateSpeed = fFreeCameraTranslationSpeed->GetFloat();
				auto [selected, hovered] = ImGui::Slider("$PM_TranslateSpeed"_T,
					&currentTranslateSpeed,  // fFreeCameraTranslationSpeed:Camera
					0.1f, 50.0f);
				if (selected) {
					fFreeCameraTranslationSpeed->SetFloat(currentTranslateSpeed);
				}
				return hovered;
			});
		}

		if (dofInstance) {
			ImGui::DisplayWidgetsGroups([&] {
				return ImGui::CheckBox("$PM_DepthOfField"_T, &enableDOF).second;
			});			
			if (!enableDOF) {
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ImGui::GetStyle().DisabledAlpha);
			}
			ImGui::BeginDisabled(!enableDOF);
			{
				ImGui::Indent();
				{
					ImGui::DisplayWidgetsGroups([&] {
						return ImGui::Slider("$PM_DOF_Strength"_T, &dofInstance->strength, 0.0f, 1.0f).second;
					});
					ImGui::DisplayWidgetsGroups([&] {
						return ImGui::Slider("$PM_DOF_Distance"_T, &dofInstance->data.depthOfField.distance, 0.0f, 5000.0f).second;
					});
					ImGui::DisplayWidgetsGroups([&] {
						return ImGui::Slider("$PM_DOF_Range"_T, &dofInstance->data.depthOfField.range, 0.0f, 5000.0f).second;
					});
					ImGui::DisplayWidgetsGroups([&] {
						return ImGui::Slider("$PM_DOF_VignetteStrength"_T, &dofInstance->data.depthOfField.vignetteStrength, 0.0f, 1.0f).second;
					});
					ImGui::DisplayWidgetsGroups([&] {
						return ImGui::Slider("$PM_DOF_VignetteRadius"_T, &dofInstance->data.depthOfField.vignetteRadius, 0.0f, 1000.0f).second;
					});
				}
				ImGui::Unindent();
			}
			ImGui::EndDisabled();
			if (!enableDOF) {
				ImGui::PopStyleVar();
			}
		}

		drawList->ChannelsMerge();
	}

	void CameraGrid::Draw()
	{
		if (gridType == kDisabled) {
			return;
		}

		const auto drawList = ImGui::GetBackgroundDrawList();

		const auto pos = ImGui::GetWindowPos();
		const auto size = ImGui::GetWindowSize();

		constexpr auto topLeft = ImVec2(0.0f, 0.0f);
		const auto static topRight = ImVec2(size.x, 0.0f);
		const auto static bottomLeft = ImVec2(0.0f, size.y);
		const auto static bottomRight = ImVec2(size.x, size.y);

		const auto colour = ImGui::GetUserStyleColorU32(ImGui::USER_STYLE::kGridLines);
		const auto thickness = ImGui::GetUserStyleVar(ImGui::USER_STYLE::kGridLines);

		switch (gridType) {
		case kRuleOfThirds:
			{
				const static auto third_width = size.x / 3;
				const static auto third_height = size.y / 3;

				// Draw the horizontal lines
				drawList->AddLine(ImVec2(0, third_height), ImVec2(size.x, third_height), colour, thickness);
				drawList->AddLine(ImVec2(0, third_height * 2), ImVec2(size.x, third_height * 2), colour, thickness);

				// Draw the vertical lines
				drawList->AddLine(ImVec2(third_width, 0), ImVec2(third_width, size.y), colour, thickness);
				drawList->AddLine(ImVec2(third_width * 2, 0), ImVec2(third_width * 2, size.y), colour, thickness);
			}
			break;
		case kDiagonal:
			{
				drawList->AddLine(topLeft, ImVec2(size.x * 0.6666f, size.y), colour, thickness);
				drawList->AddLine(topRight, ImVec2(size.x * 0.3333f, size.y), colour, thickness);

				drawList->AddLine(bottomLeft, ImVec2(size.x * 0.6666f, 0.0f), colour, thickness);
				drawList->AddLine(bottomRight, ImVec2(size.x * 0.3333f, 0.0f), colour, thickness);
			}
			break;
		case kTriangle:
			{
				static bool mirrored = false;
				if (ImGui::IsKeyPressed(ImGuiKey_LeftShift) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceLeft)) {
					mirrored = !mirrored;
				}

				// Draw main diagonal line
				drawList->AddLine(
					!mirrored ? topLeft : bottomLeft,
					!mirrored ? bottomRight : topRight,
					colour, thickness);

				drawList->AddLine(
					!mirrored ? bottomLeft : topLeft,
					!mirrored ? ImVec2(size.x * 0.30f, size.y * 0.30f) : ImVec2(size.x * 0.30f, size.y * 0.70f),
					colour, thickness);

				drawList->AddLine(
					!mirrored ? topRight : bottomRight,
					!mirrored ? ImVec2(size.x * 0.70f, size.y * 0.70f) : ImVec2(size.x * 0.70f, size.y * 0.30f),
					colour, thickness);
			}
			break;
		case kGoldenRatio:
			{
				const static auto width40 = size.x * 0.4f;
				const static auto width60 = size.x * 0.6f;

				const static auto height40 = size.y * 0.4f;
				const static auto height60 = size.y * 0.6f;

				// Draw the horizontal lines
				drawList->AddLine(ImVec2(0, height40), ImVec2(size.x, height40), colour, thickness);
				drawList->AddLine(ImVec2(0, height60), ImVec2(size.x, height60), colour, thickness);

				// Draw the vertical lines
				drawList->AddLine(ImVec2(width40, 0), ImVec2(width40, size.y), colour, thickness);
				drawList->AddLine(ImVec2(width60, 0), ImVec2(width60, size.y), colour, thickness);
			}
			break;
		/*case kGoldenSpiral:
			{
			}
			break;*/
		case kGrid:
			{
				constexpr int   GRID_WIDTH = 8;
				constexpr int   GRID_HEIGHT = 6;
				constexpr float GRID_PADDING = 20.0f;

				const float cellWidth = (size.x - 2 * GRID_PADDING) / GRID_WIDTH;
				const float cellHeight = (size.y - 2 * GRID_PADDING) / GRID_HEIGHT;

				const float startX = pos.x + GRID_PADDING - cellWidth;
				const float endX = pos.x + size.x - GRID_PADDING + cellWidth;

				const float startY = pos.y + GRID_PADDING - cellHeight;
				const float endY = pos.y + size.y - GRID_PADDING + cellHeight;

				// Draw vertical lines
				for (int i = 0; i <= GRID_WIDTH + 1; ++i) {
					const float x = startX + i * cellWidth;
					drawList->AddLine(ImVec2(x, startY), ImVec2(x, endY), colour, thickness);
				}

				// Draw horizontal lines
				for (int j = 0; j <= GRID_HEIGHT + 1; ++j) {
					const float y = startY + j * cellHeight;
					drawList->AddLine(ImVec2(startX, y), ImVec2(endX, y), colour, thickness);
				}
			}
			break;
		default:
			break;
		}
	}
}

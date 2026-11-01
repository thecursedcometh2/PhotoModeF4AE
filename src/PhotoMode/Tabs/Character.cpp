#include "PhotoMode/Tabs/Character.h"

namespace PhotoMode
{
	void Character::State::Get(const RE::Actor* a_actor)
	{
		visible = a_actor->Get3D() ? !a_actor->Get3D()->GetAppCulled() : false;

		rotZ = a_actor->data.angle.z;
		pos = a_actor->GetPosition();
	}

	void Character::RevertIdle() const
	{
		if (const auto currentProcess = character ? character->currentProcess : nullptr) {
			currentProcess->StopCurrentIdle(character, true, true);
		}
	}


	Character::Character(RE::Actor* a_actor) :
		character(a_actor)
	{
		if (a_actor->IsPlayerRef()) {
			characterName = TRANSLATE_S("$PM_Player");
		} else if (const auto actorbase = a_actor->GetNPC(); actorbase && actorbase->IsUnique()) {
			characterName = a_actor->GetDisplayFullName();
		} else {
			characterName = std::format("{} [0x{:X}]", a_actor->GetDisplayFullName(), a_actor->GetFormID());
		}

		GetOriginalState();

		if (!character->IsPlayerRef()) {
			character->InitiateDoNothingPackage();
		}
	}

	void Character::GetOriginalState()
	{
		originalState.Get(character);
        currentState.visible = originalState.visible;

		effectShaders.InitForms();
		effectVFX.InitForms();
		idles.InitForms();
	}

	void Character::RevertState()
	{
		// revert current values
		currentState.pos = RE::NiPoint3();

		if (rotationChanged) {
			character->SetHeading(originalState.rotZ);
		}
		if (positionChanged) {
			character->SetPosition(originalState.pos, true);
		}
		if (positionChanged || rotationChanged) {
			character->UpdateActor3DPosition();

			positionChanged = false;
			rotationChanged = false;
		}

		if (currentState.visible != originalState.visible) {
            if (const auto root = character->Get3D()) {
                root->CullGeometry(!originalState.visible);
            }
            currentState.visible = originalState.visible;
        }

		// revert idles
		idles.Reset();
		if (idlePlayed) {
			RevertIdle();
			idlePlayed = false;
		}

		// revert effects
		effectShaders.Reset();
		effectVFX.Reset();

		if (vfxPlayed || effectsPlayed) {
			if (const auto processLists = RE::ProcessLists::GetSingleton()) {
				const auto handle = character->GetHandle();
				processLists->ForEachMagicTempEffect([&](RE::BSTempEffect* a_effect) {
					if (const auto referenceEffect = a_effect->As<RE::ReferenceEffect>()) {
						if (referenceEffect->target == handle) {
							referenceEffect->finished = true;
						}
					}
					return RE::BSContainer::ForEachResult::kContinue;
				});
			}
			vfxPlayed = false;
			effectsPlayed = false;
		}

		if (!character->IsPlayerRef()) {
			character->EndInterruptPackage(false);
		}
	}

	const char* Character::GetName() const
	{
		return characterName.c_str();
	}

	void Character::Draw(bool a_resetTabs)
	{
		ImGui::UpdateWidgetPadding(2, "-360.00");

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSplit(2);

		ImGui::DisplayWidgetsGroups([&] {
			const std::string name = character->IsPlayerRef() ? std::string("$PM_ShowPlayer"_T) : std::format("{} {}", "$PM_ShowCharacter"_T, GetName());
			auto [selected, hovered] = ImGui::CheckBox(name.c_str(), &currentState.visible);
			if (selected) {
				if (const auto root = character->Get3D()) {
					root->CullGeometry(!currentState.visible);
				}
			}
			return hovered;
		});

		ImGui::Spacing();

		if (!currentState.visible) {
			ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ImGui::GetStyle().DisabledAlpha);
		}
		ImGui::BeginDisabled(!currentState.visible);
		{
			if (ImGui::BeginTabBar("Player#TopBar", 0)) {
				// ugly af, improve later
				const float width = ImGui::GetContentRegionAvail().x / 3;

				if (a_resetTabs) {
					ImGui::SetKeyboardFocusHere();
				}

				ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Idles"_T)) {
					idles.GetFormResultFromCombo([&](const auto& a_idle) {
						if (idlePlayed) {
							RevertIdle();
							idlePlayed = false;
						}
						if (const auto currentProcess = character->currentProcess) {
							if (currentProcess->PlayIdle(*character, a_idle, nullptr)) {
								idlePlayed = true;
							}
						}
					},
						character, false);
					ImGui::EndTabItem();
				}

				ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Effects"_T)) {
					effectShaders.GetFormResultFromCombo([&](const auto& a_effectShader) {
						character->ApplyEffectShader(a_effectShader);
						effectsPlayed = true;
					});
					effectVFX.GetFormResultFromCombo([&](const auto& a_vfx) {
						bool attachToCamera = a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kAttachToCamera);
						bool inheritRotation = a_vfx->data.flags.any(RE::BGSReferenceEffect::Flag::kInheritRotation);
						if (const auto effectShader = a_vfx->data.effectShader) {
							character->ApplyEffectShader(effectShader, -1, nullptr, attachToCamera, inheritRotation);
						}
						if (const auto artObject = a_vfx->data.artObject) {
							character->ApplyArtObject(artObject, -1, nullptr, attachToCamera, inheritRotation);
						}
						vfxPlayed = true;
					});
					ImGui::EndTabItem();
				}

				ImGui::SetNextItemWidth(width);
				if (ImGui::OpenTabOnHover("$PM_Transforms"_T)) {
					ImGui::DisplayWidgetsGroups([&] {
						currentState.rotZ = RE::rad_to_deg(character->data.angle.z);
						auto [selected, hovered] = ImGui::Slider("$PM_Rotation"_T, &currentState.rotZ, 0.0f, 360.0f);
						if (selected) {
							character->SetHeading(RE::deg_to_rad(currentState.rotZ));
							rotationChanged = true;
						}
						return hovered;
					});

					bool update = false;
					ImGui::DisplayWidgetsGroups([&] {
						auto [selected, hovered] = ImGui::Slider("$PM_PositionLeftRight"_T, &currentState.pos.x, -100.0f, 100.0f);
						update = selected;
						return hovered;
					});
					ImGui::DisplayWidgetsGroups([&] {
						auto [selected, hovered] = ImGui::Slider("$PM_PositionNearFar"_T, &currentState.pos.y, -100.0f, 100.0f);
						update |= selected;
						return hovered;
					});
					if (update) {
						character->SetPosition({ originalState.pos + currentState.pos }, true);
						positionChanged = true;
					}

					ImGui::EndTabItem();
				}
				ImGui::EndTabBar();
			}
		}
		ImGui::EndDisabled();
		if (!currentState.visible) {
			ImGui::PopStyleVar();
		}

		drawList->ChannelsMerge();
	}
}

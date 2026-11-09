#pragma once

#include "ImGui/FormComboBox.h"

namespace PhotoMode
{
	inline RE::TESIdleForm* resetRootIdle{ nullptr };
	
	class Character
	{
	public:
		Character() = default;
		Character(RE::Actor* a_actor);

		void GetOriginalState();
		void RevertState();

		const char* GetName() const;

		void Draw(bool a_resetTabs);

	private:
		struct State
		{
			void Get(const RE::Actor* a_actor);

			// members
			bool         visible{ true };
			float        rotZ{};
			RE::NiPoint3 pos{};
		};

		void RevertIdle() const;

		// members
		RE::Actor*  character{ nullptr };
		std::string characterName{};

		// Cache each character separately.
		ImGui::FormComboBoxFiltered<RE::TESEffectShader>    effectShaders{ "$PM_EffectShaders" };
		ImGui::FormComboBoxFiltered<RE::TESIdleForm>        idles{ "$PM_Idles" };
		ImGui::FormComboBoxFiltered<RE::BGSReferenceEffect> effectVFX{ "$PM_VisualEffects" };

		State originalState{};
		State currentState{};

		bool rotationChanged{ false };
		bool positionChanged{ false };

		bool effectsPlayed{ false };
		bool vfxPlayed{ false };
		bool idlePlayed{ false };
	};
}

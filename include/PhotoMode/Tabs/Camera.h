#pragma once

namespace PhotoMode
{
	void SetFOV(float a_fov);

	class Camera
	{
	public:
		void GetOriginalState();
		void RevertState(bool a_deactivate);

		[[nodiscard]] float GetViewRoll() const { return currentViewRoll; }
		[[nodiscard]] bool  GetDOFEnabled() const { return enableDOF; }

		void Draw();

	private:
		struct DOF
		{
			float strength{ 0.5 };
			float range{ 3800.0 };
			float distance{ 2000 };
		};

		struct OriginalState
		{
			void Get();
			void Revert(bool a_deactivate) const;

			// members
			float fov{};
			float translateSpeed{};
			DOF   defaultDOF;
		};

		// members
		OriginalState                                    originalState{};
		float                                            currentFOV{};
		float                                            currentViewRoll{};
		float                                            currentViewRollDegrees{};
		RE::Setting*                                     fFreeCameraTranslationSpeed{ nullptr };
		float                                            currentTranslateSpeed{};
		bool                                             enableDOF{ false };
		RE::NiPointer<RE::ImageSpaceModifierInstanceDOF> dofInstance{};
	};

	namespace CameraGrid
	{
		enum GridType
		{
			kDisabled,
			kRuleOfThirds,
			kDiagonal,
			kTriangle,
			kGoldenRatio,
			//kGoldenSpiral,
			kGrid
		};

		static constexpr std::array gridTypes{
			"$NONE",
			"$PM_Grid_Thirds",
			"$PM_Grid_Diagonal",
			"$PM_Grid_Triangle",
			"$PM_Grid_GoldenRatio",
			/*"$PM_Grid_GoldenSpiral"*/
			"$PM_Grid_Grid"
		};

		void Draw();

		// members
		inline GridType gridType{ kDisabled };
	}
}

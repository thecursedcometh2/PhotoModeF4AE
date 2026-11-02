#pragma once
#include <cstdint>
#include <filesystem>
#include <mutex>

namespace Screenshot
{
	class Manager final : public REX::TSingleton<Manager>
	{
	public:
		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void LoadScreenshots();

		bool TakeScreenshot();

		std::uint64_t GetIndex() const;
		void          AssignHighestPossibleIndex();
		void          IncrementIndex();

		bool AllowMultiScreenshots() const;
		bool CanAutoHideMenus() const;

	private:
		// members
		std::uint64_t index{ 0 };
        std::mutex screenshotMutex;
        std::filesystem::path DestinationPrefix() const;

		bool allowMultiScreenshots{ true };
		bool autoHideMenus{ true };

		bool                  useCustomFolderDirectory{ true };
		std::filesystem::path photoDirectory{};
	};
}

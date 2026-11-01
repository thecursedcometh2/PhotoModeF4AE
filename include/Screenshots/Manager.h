#pragma once

namespace Screenshot
{
	// 48
	struct Image
	{
		Image() = default;
		Image(std::string_view a_path, std::uint32_t a_index);
		Image(std::string& a_path);

		bool operator<(const Image& a_rhs) const
		{
			return index < a_rhs.index;
		}

		// members
		std::string  path{};
		std::int32_t index{ -1 };
	};
	
	class Manager final : public REX::TSingleton<Manager>
	{
	public:
		void LoadMCMSettings(const CSimpleIniA& a_ini);
		void LoadScreenshots();

		bool TakeScreenshot();

		std::uint32_t GetIndex() const;
		void          AssignHighestPossibleIndex();
		void          IncrementIndex();

		bool AllowMultiScreenshots() const;
		bool CanAutoHideMenus() const;

	private:
		// members
		std::int32_t index{ -1 };

		bool allowMultiScreenshots{ true };
		bool autoHideMenus{ true };

		bool                  useCustomFolderDirectory{ true };
		std::filesystem::path photoDirectory{};
	};
}

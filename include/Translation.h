#pragma once
#include "EnglishTranslations.h"
#include <map>

namespace Translation
{
	class Manager final : public REX::TSingleton<Manager>
	{
	public:
        Manager() { for (const auto& [key, value] : EnglishDefaults) translationMap.emplace(key, value); }
		static std::string GetGameLanguage();

		void BuildTranslationMap();
		bool LoadTranslation(const std::filesystem::path& a_path);
		void AddGameTranslationStrings();

		template <class T>
		const std::string& GetTranslation(const T& a_key)
		{
			if (const auto it = translationMap.find(a_key); it != translationMap.end()) {
				return it->second;
			}

			// Retain a distinct, stable string even for unknown third-party keys.
            return missingTranslations.try_emplace(std::string(a_key), std::string(a_key)).first->second;
		}

	private:
		StringMap<std::string> translationMap{};
        std::map<std::string, std::string, std::less<>> missingTranslations;
	};
}

#define TRANSLATE(STR) Translation::Manager::GetSingleton()->GetTranslation(STR).c_str()
#define TRANSLATE_S(STR) Translation::Manager::GetSingleton()->GetTranslation(STR)

constexpr const char* operator""_T(const char* str, std::size_t)
{
	return Translation::Manager::GetSingleton()->GetTranslation(str).c_str();
}

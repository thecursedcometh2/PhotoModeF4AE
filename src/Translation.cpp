#include "Translation.h"
#include "TranslationLanguage.h"

namespace Translation
{
	std::string Manager::GetGameLanguage()
	{
		return ResolveGameLanguage([](std::string_view name) { return RE::GetINISetting(name); });
	}

	void Manager::BuildTranslationMap()
	{
        LoadTranslation(R"(Data\Interface\Translations\PhotoMode_MCMFallback_EN.txt)");
        const std::filesystem::path english(R"(Data\Interface\Translations\PhotoMode_EN.txt)");
        if (!LoadTranslation(english)) {
            logger::info("Missing translation file {}. Using English.", english.string());
        }
        gameLanguage = GetGameLanguage();
        const auto& language = gameLanguage;
        if (language != "EN") {
            const auto localized = std::filesystem::path(std::format(R"(Data\Interface\Translations\PhotoMode_{}.txt)", language));
            if (!LoadTranslation(localized)) logger::info("Missing translation file {}. Keeping English.", localized.string());
        }

		AddGameTranslationStrings();
	}

	bool Manager::LoadTranslation(const std::filesystem::path& a_path)
	{
		std::error_code ec;
		if (!std::filesystem::exists(a_path, ec)) {
			return false;
		}

		std::wfstream filestream(a_path, std::wfstream::in | std::wfstream::binary);
		if (!filestream.good()) {
			return false;
		} else {
			logger::info("Reading translations from {}...", a_path.string());
		}

		filestream.imbue(std::locale(filestream.getloc(), new std::codecvt_utf16<wchar_t, 0x10FFFF, std::little_endian>));

		// check if the BOM is UTF-16
		constexpr wchar_t BOM_UTF16LE = 0xFEFF;
		if (filestream.get() != BOM_UTF16LE) {
			logger::info("Translation file needs UTF-16LE with a BOM.");
			return false;
		}

		std::wstring line, key, value;

		while (std::getline(filestream, line)) {
            key.clear(); value.clear();
			std::wstringstream ss(line);
			ss >> key;
			// remove leading whitespace
			std::getline(ss >> std::ws, value);
			// remove space/new line at end
			if (!value.empty() && std::iswspace(value.back())) {
				value.pop_back();
			}
			const auto utfKey = stl::utf16_to_utf8(key);
            const auto utfValue = stl::utf16_to_utf8(value);
            if (utfKey && utfValue && !utfKey->empty()) {
                if (!utfValue->empty()) translationMap.insert_or_assign(*utfKey, *utfValue);
            }
		}

		return true;
	}

	void Manager::AddGameTranslationStrings()
	{
		const auto scaleformManager = RE::BSScaleformManager::GetSingleton();
		const auto loader = scaleformManager ? scaleformManager->loader : nullptr;
		const auto translator = loader ? static_cast<RE::BSScaleformTranslator*>(loader->GetStateAddRef(Scaleform::GFx::State::StateType::kTranslator)) : nullptr;

		if (!translator) {
            logger::error("MCM translator unavailable. Keeping file translations.");
            return;
        }

        translator->AddTranslationsFile("Interface\\Translations\\PhotoMode_MCMFallback_EN.txt");
        translator->AddTranslationsFile("Interface\\Translations\\PhotoMode_EN.txt");
        if (gameLanguage != "EN") {
            const auto localized = std::format("Interface\\Translations\\PhotoMode_{}.txt", gameLanguage);
            translator->AddTranslationsFile(localized.c_str());
        }

        auto FetchTranslation = [this](RE::BSScaleformTranslator* a_trns, const wchar_t* a_key) {
			auto it = a_trns->translator.translationMap.find(a_key);
			if (it != a_trns->translator.translationMap.end()) {
				translationMap.insert_or_assign(*stl::utf16_to_utf8(a_key), *stl::utf16_to_utf8(it->second.data()));
			}
		};

		FetchTranslation(translator, L"$ON");
		FetchTranslation(translator, L"$OFF");
		FetchTranslation(translator, L"$EXIT");
		FetchTranslation(translator, L"$YES");
		FetchTranslation(translator, L"$NO");
		FetchTranslation(translator, L"$RESET");
		FetchTranslation(translator, L"$NONE");
	}
}

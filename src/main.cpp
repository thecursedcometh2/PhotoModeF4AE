#include "Hooks.h"
#include "ImGui/Renderer.h"
#include "Input.h"
#include "Console.h"
#include "PhotoMode/Manager.h"
#include "Screenshots/Manager.h"
#include "Settings.h"
#include "Translation.h"

void MessageHandler(F4SE::MessagingInterface::Message* a_message)
{
	switch (a_message->type) {
	case F4SE::MessagingInterface::kPostLoad:
		{
			logger::info("{:*^30}", "POST LOAD");
			ImGui::Renderer::Install();
			Hooks::Install();
			Console::Install();
		}
		break;
	case F4SE::MessagingInterface::kGameDataReady:
		{
			logger::info("{:*^30}", "DATA LOADED");

			ImGui::Renderer::Init();
			
			MANAGER(Input)->Register();
			MANAGER(PhotoMode)->Register();
			
			MANAGER(Translation)->BuildTranslationMap();
			MANAGER(Screenshot)->LoadScreenshots();
			MANAGER(PhotoMode)->OnDataLoad();
		}
		break;
	default:
		break;
	}
}

extern "C" DLLEXPORT bool F4SEAPI F4SEPlugin_Load(const F4SE::LoadInterface* a_f4se)
{
	if (a_f4se->RuntimeVersion() != F4SE::RUNTIME_1_11_240) {
        return false;
    }
    // The official plugin rule generates the version export and Windows resources.
    static_assert(F4SE::RUNTIME_LATEST == F4SE::RUNTIME_1_11_240);
    F4SE::Init(a_f4se, {.logName = "po3_PhotoModeF4", .trampoline = true, .trampolineSize = 128});

	logger::info("Game version : {}", a_f4se->RuntimeVersion().string());

	Settings::GetSingleton()->LoadMCMSettings();

	const auto messaging = F4SE::GetMessagingInterface();
	if (!messaging || !messaging->RegisterListener(MessageHandler)) {
        return false;
    }

	return true;
}

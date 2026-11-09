#include "Console.h"
#include "PlayerSyncRecovery.h"

#include "PhotoMode/Manager.h"

namespace Console
{
	namespace detail
	{
		constexpr auto LONG_NAME = "PhotoMode"sv;
		constexpr auto SHORT_NAME = ""sv;

		[[nodiscard]] const std::string& HelpString()
		{
			static auto help = []() {
				std::string buf;
				buf += "PhotoMode: toggle. PhotoMode 1: fix stuck third-person movement.\n";
				return buf;
			}();
			return help;
		}

        static void RecoverPlayerSync()
        {
            const auto player = RE::PlayerCharacter::GetSingleton();
            const auto camera = RE::PlayerCamera::GetSingleton();
            const bool permitted = player && camera && camera->currentState &&
                camera->currentState->id == RE::CameraState::k3rdPerson &&
                !MANAGER(PhotoMode)->IsActive() && !PhotoMode::Manager::IsScreenArcherMenuOpen() &&
                !player->inSyncAnim && static_cast<unsigned>(player->interactingState) == 0 &&
                !player->boolFlags.any(RE::Actor::BOOL_FLAGS::kIsInKillMove);
            struct Graph {
                RE::PlayerCharacter* player;
                bool Get(const char* name, bool& value) { return player->GetGraphVariableImplBool(RE::BSFixedString(name), value); }
                bool Set(const char* name, bool value) { return player->SetGraphVariableBool(RE::BSFixedString(name), value); }
            } graph{player};
            const auto result = PhotoMode::ClearPlayerSync(graph, permitted);
            constexpr const char* messages[] = {
                "Third Person Fix unavailable. Use normal third person. Exit Photo Mode and SAM.",
                "Third Person Fix: animation data unavailable.",
                "Third Person Fix: no change needed.",
                "Third Person Fix failed.",
                "Third Person Fix: sync flag is still set.",
                "Third Person Fix applied. Test movement."
            };
            const auto message = messages[static_cast<unsigned>(result)];
            logger::info("{}", message);
            if (const auto console = RE::ConsoleLog::GetSingleton()) console->Log("{}", message);
        }

        static bool Execute(const RE::SCRIPT_PARAMETER* parameters, const char* compiled,
            RE::TESObjectREFR* ref, RE::TESObjectREFR* container, RE::Script* script,
            RE::ScriptLocals* locals, float& result, std::uint32_t& offset)
        {
            std::int32_t action = 0;
            if (!RE::Script::ParseParameters(parameters, compiled, offset, ref, container, script, locals, &action)) return false;
            result = 0.0f;
            if (action == 0) MANAGER(PhotoMode)->ToggleActive();
            else if (action == 1) RecoverPlayerSync();
            else if (const auto console = RE::ConsoleLog::GetSingleton()) console->Log("Usage: PhotoMode [0=toggle, 1=player sync recovery]");
            return true;
        }

	}

    namespace
    {
        class ThirdPersonFixHandler final : public Scaleform::GFx::FunctionHandler
        {
        public:
            void Call(const Params& params) override
            {
                if (params.argCount != 0) return;
                if (const auto tasks = F4SE::GetTaskInterface()) {
                    tasks->AddTask([] { detail::RecoverPlayerSync(); });
                }
            }
        };

        bool RegisterMCMFunctions(Scaleform::GFx::Movie* movie, Scaleform::GFx::Value* plugin)
        {
            if (!movie || !plugin) return false;
            auto* handler = new ThirdPersonFixHandler();
            Scaleform::GFx::Value function;
            movie->CreateFunction(&function, handler);
            handler->Release();
            return plugin->SetMember("ThirdPersonFix", function);
        }
    }

    bool RegisterMCM()
    {
        const auto scaleform = F4SE::GetScaleformInterface();
        return scaleform && scaleform->Register("po3_PhotoModeF4", RegisterMCMFunctions);
    }

	void Install()
	{
		const auto functions = RE::SCRIPT_FUNCTION::GetConsoleFunctions();
		const auto it = std::find_if(
			functions.begin(),
			functions.end(),
			[&](auto&& a_elem) {
				return _stricmp(a_elem.functionName, "TogglePathLine") == 0;
			});
		if (it != functions.end()) {
			*it = RE::SCRIPT_FUNCTION{ detail::LONG_NAME.data(), detail::SHORT_NAME.data(), it->output };
			it->helpString = detail::HelpString().data();
			it->referenceFunction = false;
            static RE::SCRIPT_PARAMETER parameters[]{ {"action", RE::SCRIPT_PARAM_TYPE::kInt, true} };
            it->SetParameters(parameters);
			it->executeFunction = &detail::Execute;
			it->conditionFunction = nullptr;

			logger::info("Installed {}", detail::LONG_NAME);
		} else {
			stl::report_and_fail("failed to find function"sv);
		}
	}
}

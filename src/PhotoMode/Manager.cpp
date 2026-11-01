#include "PhotoMode/Manager.h"

#include "ImGui/Fonts/Manager.h"
#include "ImGui/Styles.h"
#include "ImGui/Widgets.h"
#include "PhotoMode/Hotkeys.h"
#include "Screenshots/Manager.h"
#include "Settings.h"

#include "Input.h"

namespace PhotoMode
{
	void Manager::Register()
	{
		RE::UI::GetSingleton()->RegisterSink<RE::MenuOpenCloseEvent>(GetSingleton());
	}

	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		freeCameraSpeed = a_ini.GetDoubleValue("Settings", "fFreeCameraTranslationSpeed", freeCameraSpeed);
		freezeTimeOnStart = a_ini.GetBoolValue("Settings", "bFreezeTimeOnStart", freezeTimeOnStart);
		openFromPauseMenu = a_ini.GetBoolValue("Settings", "bOpenFromPauseMenu", openFromPauseMenu);
	}

	bool Manager::IsValid()
	{
		const auto* controlMap = RE::ControlMap::GetSingleton();
		if (!controlMap || controlMap->contextPriorityStack.empty()) {
			return false;
		}

		constexpr std::array allowedContexts{
			RE::UserEvents::INPUT_CONTEXT_ID::kMainGameplay,
			RE::UserEvents::INPUT_CONTEXT_ID::kSitWait,
			RE::UserEvents::INPUT_CONTEXT_ID::kTFC,
			RE::UserEvents::INPUT_CONTEXT_ID::kConsole
		};

		const auto& context_id = controlMap->contextPriorityStack.back();
		if (!std::ranges::contains(allowedContexts, context_id)) {
			return false;
		}

		return true;
	}

	bool Manager::ShouldBlockInput() const
	{
		return blockInput;
	}

	bool Manager::IsActive() const
	{
		return activated;
	}

	bool Manager::IsHidden() const
	{
		return hiddenUI;
	}

	void Manager::ToggleUI()
	{
		hiddenUI = !hiddenUI;

		const auto UI = RE::UI::GetSingleton();
		UI->menuSystemVisible = !UI->menuSystemVisible;
		RE::Interface3D::Renderer::DisableAll(!UI->menuSystemVisible);

		RE::UIUtils::PlayMenuSound("UIMenuFocus");

		if (!hiddenUI) {
			restoreLastFocusID = true;
		}
	}

	void Manager::Activate()
	{
		cameraTab.GetOriginalState();
		timeTab.GetOriginalState();

		const auto player = RE::PlayerCharacter::GetSingleton();
		characterTab.emplace(player->GetFormID(), Character(player));
		cachedCharacter = player;

		filterTab.GetOriginalState();

		const auto pcCamera = RE::PlayerCamera::GetSingleton();
		originalcameraState = pcCamera->currentState ? pcCamera->currentState->id.get() : RE::CameraState::k3rdPerson;

		menusAlreadyHidden = !RE::UI::GetSingleton()->menuSystemVisible;
		if (menusAlreadyHidden) {
			hiddenUI = true;
		}

		// Preserve pre-existing restrictions owned by the game or another mod.
        originalSavingAllowed = RE::BGSSaveLoadManager::GetSingleton()->savingAllowed;
        RE::BGSSaveLoadManager::GetSingleton()->savingAllowed = false;

		// toggle freecam
		if (originalcameraState != RE::CameraState::kFree) {
			pcCamera->ToggleFreeCameraMode(false);
		}

		// disable controls
		TogglePlayerControls(false);

		// apply mcm settings
		RE::INISettingCollection::GetSingleton()->GetSetting("fFreeCameraTranslationSpeed:Camera")->SetFloat(freeCameraSpeed);
		if (freezeTimeOnStart) {
			RE::Main::GetSingleton()->freezeTime = true;
		}

		// refresh style
		ImGui::Styles::GetSingleton()->OnStyleRefresh();

		// toggle HUD elements
		RE::SendHUDMessage::PushHUDMode(RE::HUDModeType("DialogueCameraMode"));
		if (RE::UI::GetSingleton()->GetMenuOpen<RE::SitWaitMenu>()) {
			RE::UIMessageQueue::GetSingleton()->AddMessage("SitWaitMenu", RE::UI_MESSAGE_TYPE::kHide);
			sitWaitMenuOpen = true;
		}

		activated = true;
		if (activeGlobal) {
			activeGlobal->value = 1.0f;
		}
	}

	void Manager::TogglePlayerControls(bool a_enable)
	{
		if (!inputLayer) {
			RE::BSInputEnableManager::GetSingleton()->AllocateNewLayer(inputLayer, "PhotoMode");
		}
		if (inputLayer) {
			RE::BSInputEnableManager::GetSingleton()->EnableUserEvent(inputLayer->layerID, controlFlags.get(), a_enable, RE::UserEvents::SENDER_ID::kScript);
		}

        if (!a_enable) originalHandlers.fill(nullptr);
        if (const auto controls = RE::PlayerControls::GetSingleton()) {
            const std::array<RE::BSInputEventUser*, 5> handlers{
                controls->sneakHandler, controls->jumpHandler, controls->attackHandler,
                controls->readyWeaponHandler, controls->meleeThrowHandler};
            for (std::size_t i = 0; i < handlers.size(); ++i) {
                if (!handlers[i]) continue;
                if (!a_enable) {
                    originalHandlers[i] = handlers[i];
                    originalHandlerStates[i] = handlers[i]->inputEventHandlingEnabled;
                    handlers[i]->inputEventHandlingEnabled = false;
                } else if (handlers[i] == originalHandlers[i]) {
                    handlers[i]->inputEventHandlingEnabled = originalHandlerStates[i];
                }
            }
        }
        if (a_enable) originalHandlers.fill(nullptr);
	}

	bool Manager::OnFrameUpdate()
	{
		if (!IsValid()) {
			Deactivate();
			return false;
		}

		// disable controls
		if (ImGui::GetIO().WantTextInput) {
			if (!allowTextInput) {
				allowTextInput = true;
				RE::ControlMap::GetSingleton()->SetTextEntryMode(true);
			}
		} else if (allowTextInput) {
			allowTextInput = false;
			RE::ControlMap::GetSingleton()->SetTextEntryMode(false);
		}

		timeTab.OnFrameUpdate();

		return true;
	}

	void Manager::Deactivate()
	{
		Revert(true);

		//reset characters
		characterTab.clear();
		cachedCharacter = nullptr;

		// reset camera
		if (originalcameraState != RE::CameraState::kFree) {
			RE::PlayerCamera::GetSingleton()->ToggleFreeCameraMode(false);
			//RE::ControlMap::GetSingleton()->PopInputContext(RE::ControlMap::InputContextID::kTFCMode);
		}

		// reset controls
		if (allowTextInput) {
            RE::ControlMap::GetSingleton()->SetTextEntryMode(false);
            allowTextInput = false;
        }
		TogglePlayerControls(true);

		// allow saving
		RE::BGSSaveLoadManager::GetSingleton()->savingAllowed = originalSavingAllowed;

		// restore HUD elements
		RE::SendHUDMessage::PopHUDMode(RE::HUDModeType("DialogueCameraMode"));
		if (sitWaitMenuOpen) {
			auto player = RE::PlayerCharacter::GetSingleton();
			if (player->currentProcess) {
				RE::SitWaitMenu::OnEnterFurniture(player->currentProcess->GetOccupiedFurniture());
			}
			sitWaitMenuOpen = false;
		}

		// VATS INI setting was never changed on entry; leave the game setting intact.

		// reset variables
		hiddenUI = false;

		noItemsFocused = false;
		restoreLastFocusID = false;
		lastFocusedID = 0;

		updateKeyboardFocus = false;

		MANAGER(Input)->ResetInputDevices();

		activated = false;
		if (activeGlobal) {
			activeGlobal->value = 0.0f;
		}
	}

	bool Manager::ToggleActive()
	{
		if (!IsActive()) {
			if (IsValid() && !ShouldBlockInput()) {
				RE::UIUtils::PlayMenuSound("UIMenuOK");
				Activate();
				return true;
			}
		} else {
			if (!ImGui::GetIO().WantTextInput && !ShouldBlockInput()) {
				Deactivate();
				RE::UIUtils::PlayMenuSound("UIMenuCancel");
				return true;
			}
		}

		return false;
	}

	bool Manager::CanActivateFromPauseMenu() const
	{
		return openFromPauseMenu;
	}

	void Manager::ActivateFromPauseMenu()
	{
		RE::UIMessageQueue::GetSingleton()->AddMessage("PauseMenu", RE::UI_MESSAGE_TYPE::kHide);
		RE::UIUtils::PlayMenuSound("UIMenuOK");
		Activate();
	}

	void Manager::Revert(bool a_deactivate)
	{
		const std::int32_t tabIndex = a_deactivate ? -1 : currentTab;

		// Camera
		if (tabIndex == -1 || tabIndex == kCamera) {
			cameraTab.RevertState(a_deactivate);
			if (!a_deactivate) {
				RE::GetINISetting("fFreeCameraTranslationSpeed:Camera")->SetFloat(freeCameraSpeed);
			}
		}
		// Time/Weather
		if (tabIndex == -1 || tabIndex == kTime) {
			timeTab.RevertState();
		}

		// Character
		if (tabIndex == kCharacter) {
			if (cachedCharacter) {
				characterTab[cachedCharacter->GetFormID()].RevertState();
			}
		} else if (tabIndex == -1) {
			std::for_each(characterTab.begin(), characterTab.end(), [](auto& data) {
				data.second.RevertState();
			});
		}

		// Filters
		if (tabIndex == -1 || tabIndex == kFilters || tabIndex == kCamera) {
			filterTab.RevertState(tabIndex == -1);
		}
		// Overlays
		if (tabIndex == -1 || tabIndex == kOverlays) {
			overlaysTab.RevertOverlays();
		}

		if (a_deactivate) {
			// reset UI
			RE::UI::GetSingleton()->menuSystemVisible = !menusAlreadyHidden;
            RE::Interface3D::Renderer::DisableAll(menusAlreadyHidden);
			resetWindow = true;
			resetPlayerTabs = true;
		} else {
			RE::UIUtils::PlayMenuSound("UIMenuOK");

			const auto notification = std::format("{}", resetAll ? "$PM_ResetNotifAll"_T : TRANSLATE(tabResetNotifs[currentTab]));
			RE::SendHUDMessage::ShowHUDMessage(notification.c_str(), nullptr, true, false);

			if (resetAll) {
				resetAll = false;
			}
		}
	}

	void Manager::QuitOnEscape()
	{
		if (IsHidden() || noItemsFocused) {
			Deactivate();
			RE::UIUtils::PlayMenuSound("UIMenuCancel");
		}
	}

	bool Manager::GetResetAll() const
	{
		return resetAll;
	}

	void Manager::DoResetAll()
	{
		resetAll = true;
	}

	void Manager::NavigateTab(bool a_left)
	{
		if (a_left) {
			currentTab = (currentTab - 1 + tabs.size()) % tabs.size();
		} else {
			currentTab = (currentTab + 1) % tabs.size();
		}
		updateKeyboardFocus = true;
	}

	float Manager::GetViewRoll(const float a_fallback) const
	{
		return IsActive() ? cameraTab.GetViewRoll() : a_fallback;
	}

	bool Manager::GetDOFEnabled() const
	{
		return cameraTab.GetDOFEnabled();
	}

	void Manager::OnDataLoad()
	{
		overlaysTab.LoadOverlays();
		activeGlobal = RE::TESForm::GetFormByEditorID<RE::TESGlobal>("PhotoMode_IsActive");
	}

	std::pair<ImGui::Texture*, float> Manager::GetOverlay() const
	{
		return overlaysTab.GetCurrentOverlay();
	}

	void Manager::Draw()
	{
		ImGui::SetNextWindowPos(ImGui::GetNativeViewportPos());
		ImGui::SetNextWindowSize(ImGui::GetNativeViewportSize());

		ImGui::Begin("##Main", nullptr, ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoBackground | ImGuiWindowFlags_NoDecoration);
		{
			bool shouldBlockInput = ShouldBlockInput();

			if (shouldBlockInput) {
				ImGui::PushStyleVar(ImGuiStyleVar_DisabledAlpha, ImGui::GetStyle().Alpha);
			}

			ImGui::BeginDisabled(shouldBlockInput);
			{
				ImGui::PushFont(NULL, MANAGER(Font)->GetDefaultFontSize().first);
				{
					// render hierachy
					overlaysTab.DrawOverlays();

					if (!IsHidden()) {
						CameraGrid::Draw();
						DrawBar();
						DrawControls();
					}
				}
				ImGui::PopFont();
			}
			ImGui::EndDisabled();

			if (shouldBlockInput) {
				ImGui::PopStyleVar(ImGuiStyleVar_DisabledAlpha);
			}
		}
		ImGui::End();
	}

	void Manager::DrawControls()
	{
		const auto io = ImGui::GetIO();

		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto size = ImGui::GetNativeViewportSize();

		const static auto third_width = size.x / 3;
		const static auto third_height = size.y / 3;

		ImGui::SetNextWindowPos(ImVec2(center.x + third_width, center.y + third_height * 0.8f), ImGuiCond_Always, ImVec2(0.5, 0.5));
		ImGui::SetNextWindowSize(ImVec2(size.x / 3.25f, size.y / 3.125f));

		constexpr auto windowFlags = ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_NoMouseInputs | ImGuiWindowFlags_NoDecoration;

		ImGui::Begin("$PM_Title_Menu"_T, nullptr, windowFlags);
		{
			ImGui::ExtendWindowPastBorder();

			ImGui::Spacing();

			if (resetWindow) {
				currentTab = kCamera;
			}

			// Q [Tab Tab Tab Tab Tab] E
			ImGui::BeginGroup();
			{
				ImGui::AlignTextToFramePadding();

				ImGui::SameLine();
				auto buttonSize = ImGui::ButtonIcon(MANAGER(Hotkeys)->PreviousTabKey());
				ImGui::SameLine();

				const float tabWidth = (ImGui::GetContentRegionAvail().x - (buttonSize.x + buttonSize.x + tabs.size())) / tabs.size();

				ImGui::PushItemFlag(ImGuiItemFlags_NoNav, true);
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 0));

				ImDrawList* drawList = ImGui::GetWindowDrawList();
				drawList->ChannelsSplit(2);
				auto selectedBG = ImGui::GetUserStyleColorU32(ImGui::USER_STYLE::kSelectedBG);

				auto [defaultSize, largeSize] = MANAGER(Font)->GetDefaultFontSize();
				auto [defaultIconSize, largeIconSize] = MANAGER(Font)->GetDefaultIconSize();

				ImGui::PushFont(NULL, largeSize);
				for (std::int32_t i = 0; i < tabs.size(); ++i) {
					drawList->ChannelsSetCurrent(1);
					if (currentTab == i) {
						ImGui::PushStyleColor(ImGuiCol_Text, ImGui::GetColorU32(ImGuiCol_TextDisabled));
						ImGui::PushStyleColor(ImGuiCol_TextShadow, ImGui::GetColorU32(ImGuiCol_TextShadowDisabled));
					}
					ImGui::PushFont(NULL, currentTab == i ? largeIconSize : defaultIconSize);
					ImGui::Button(tabIcons[i], ImVec2(tabWidth, ImGui::GetFrameHeightWithSpacing()));
					ImGui::PopFont();
					if (currentTab == i) {
						ImGui::PopStyleColor(2);
						drawList->ChannelsSetCurrent(0);
						ImGui::SelectableColorRect(selectedBG);
					}
					ImGui::SameLine();
				}
				ImGui::PopFont();

				drawList->ChannelsMerge();
				ImGui::PopStyleColor();
				ImGui::PopItemFlag();

				ImGui::SameLine();
				ImGui::ButtonIcon(MANAGER(Hotkeys)->NextTabKey());
			}
			ImGui::EndGroup();

			ImGui::Spacing();
			ImGui::Spacing();
			ImGui::SeparatorEx(ImGuiSeparatorFlags_Horizontal, ImGui::GetStyle().WindowBorderSize);

			// content
			ImGui::SetNextWindowBgAlpha(0.0f);  // child bg color is added ontop of window
			ImGui::BeginChild("##PhotoModeChild", ImVec2(0, 0), false, windowFlags);
			{
				ImGui::Spacing();
				ImGui::Spacing();

				if (restoreLastFocusID) {
					ImGui::SetFocusID(lastFocusedID, ImGui::GetCurrentWindow());

					restoreLastFocusID = false;
				} else if (updateKeyboardFocus) {
					if (currentTab == TAB_TYPE::kCharacter) {
						resetPlayerTabs = true;
					}

					ImGui::SetKeyboardFocusHere();
					RE::UIUtils::PlayMenuSound("UIJournalTabsSD");

					updateKeyboardFocus = false;
				}

				ImGui::PushStyleVarY(ImGuiStyleVar_ItemSpacing, ImGui::GetStyle().ItemSpacing.y * 2.0f);

				switch (currentTab) {
				case TAB_TYPE::kCamera:
					{
						if (resetWindow) {
							ImGui::SetKeyboardFocusHere();
							resetWindow = false;
						}
						cameraTab.Draw();
					}
					break;
				case TAB_TYPE::kTime:
					timeTab.Draw();
					break;
				case TAB_TYPE::kCharacter:
					{
						const auto consoleRef = RE::Console::GetCurrentPickREFR().get();
						if (!consoleRef || !consoleRef->Is(RE::FormType::kACHR) || consoleRef->IsDisabled() || consoleRef->IsDeleted() || !consoleRef->Is3rdPersonVisible()) {
							prevCachedCharacter = cachedCharacter;
							cachedCharacter = RE::PlayerCharacter::GetSingleton();
						} else {
							prevCachedCharacter = cachedCharacter;
							cachedCharacter = consoleRef->As<RE::Actor>();
							if (!characterTab.contains(cachedCharacter->GetFormID())) {
								characterTab.emplace(cachedCharacter->GetFormID(), Character(cachedCharacter));
							}
						}

						if (cachedCharacter != prevCachedCharacter) {
							resetPlayerTabs = true;
						}

						characterTab[cachedCharacter->GetFormID()].Draw(resetPlayerTabs);

						if (resetPlayerTabs) {
							resetPlayerTabs = false;
						}
					}
					break;
				case TAB_TYPE::kFilters:
					filterTab.Draw();
					break;
				case TAB_TYPE::kOverlays:
					overlaysTab.Draw();
					break;
				default:
					break;
				}

				ImGui::PopStyleVar();

				noItemsFocused = !ImGui::IsAnyItemFocused() && !io.WantTextInput || !ImGui::IsWindowFocused();
				lastFocusedID = ImGui::GetFocusID();
			}
			ImGui::EndChild();
		}
		ImGui::End();
	}

	void Manager::DrawBar() const
	{
		const static auto center = ImGui::GetNativeViewportCenter();
		const static auto size = ImGui::GetNativeViewportSize();
		const static auto offsetY = size.y / 25.0f;

		ImGui::SetNextWindowPos(ImVec2(center.x, size.y - offsetY), ImGuiCond_Always, ImVec2(0.5, 0.5));

		ImGui::Begin("##Bar", nullptr, ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize);  // same offset as control window
		{
			ImGui::ExtendWindowPastBorder();

			const static auto takePhotoLabel = "$PM_TAKEPHOTO"_T;
			const static auto toggleMenusLabel = "$PM_TOGGLEMENUS"_T;
			const auto        resetLabel = GetResetAll() ? "$PM_RESET_ALL"_T : "$RESET"_T;
			const static auto freezeTimeLabel = "$PM_FREEZETIME"_T;

			const auto& takePhotoIcon = MANAGER(Hotkeys)->TakePhotoIcon();
			const auto& toggleMenusIcon = MANAGER(Hotkeys)->ToggleMenusIcon();
			const auto& resetIcon = MANAGER(Hotkeys)->ResetIcon();
			const auto& freezeTimeIcon = MANAGER(Hotkeys)->FreezeTimeIcon();

			// const static auto togglePMLabel = "$EXIT"_T;
			// const auto& togglePMIcons = MANAGER(Hotkeys)->TogglePhotoModeIcons();

			// calc total elements width
			const ImGuiStyle& style = ImGui::GetStyle();

			float width = 0.0f;

			const auto calc_width = [&](const Font::Button& a_icon, const char* a_textLabel) {
				width += ImGui::CalcTextSize(a_icon.first.c_str()).x;
				width += style.ItemSpacing.x;
				width += ImGui::CalcTextSize(a_textLabel).x;
				width += style.ItemSpacing.x;
			};

			calc_width(takePhotoIcon, takePhotoLabel);
			calc_width(toggleMenusIcon, toggleMenusLabel);
			calc_width(freezeTimeIcon, freezeTimeLabel);
			calc_width(resetIcon, resetLabel);

			/*for (const auto& icon : togglePMIcons) {
				width += icon->size.x;
			}
			width += style.ItemSpacing.x;
			width += ImGui::CalcTextSize(togglePMLabel).x;*/

			// align at center
			ImGui::AlignForWidth(width, 0.0f);

			// draw
			constexpr auto draw_button = [](const Font::Button& a_icon, const char* a_textLabel) {
				ImGui::ButtonIconWithLabel(a_textLabel, a_icon, true);
				ImGui::SameLine();
			};

			draw_button(takePhotoIcon, takePhotoLabel);
			draw_button(toggleMenusIcon, toggleMenusLabel);
			draw_button(freezeTimeIcon, freezeTimeLabel);
			draw_button(resetIcon, resetLabel);

			// ImGui::ButtonIconWithLabel(togglePMLabel, togglePMIcons, true);
		}
		ImGui::End();
	}

	bool Manager::SetupPauseMenu() const
	{
		if (auto menu = RE::UI::GetSingleton()->GetMenu<RE::PauseMenu>()) {
			if (const auto& movie = menu->uiMovie) {
				Scaleform::GFx::Value entryList;
				if (movie->GetVariable(&entryList, "root1.Menu_mc.MainPanel_mc.List_mc.entryList")) {
					std::optional<std::uint32_t> modMenuIndex = std::nullopt;

					const auto size = entryList.GetArraySize();
					for (std::uint32_t i = 0; i < size; i++) {
						Scaleform::GFx::Value entry;
						if (!entryList.GetElement(i, &entry)) {
							continue;
						}
						Scaleform::GFx::Value entryText;
						if (!entry.GetMember("text", &entryText)) {
							continue;
						}
						if (const std::string text = entryText.GetString(); text == "$INSTALLED CONTENT") {
							modMenuIndex = i;
							break;
						}
					}

					if (modMenuIndex) {
						Scaleform::GFx::Value entry;
						movie->CreateObject(&entry);
						entry.SetMember("text", "$PM_Title_Menu");
						entry.SetMember("disabled", RE::UI::GetSingleton()->GetMenuOpen<RE::PipboyMenu>());
						entry.SetMember("index", 14.0);
						entry.SetMember("clipIndex", (std::uint32_t)4);

						entryList.SetElement(*modMenuIndex, entry);
						entryList.Invoke("InvalidateData");

						return true;
					}
				}
			}
		}

		return false;
	}

	EventResult Manager::ProcessEvent(const RE::MenuOpenCloseEvent& a_event, RE::BSTEventSource<RE::MenuOpenCloseEvent>*)
	{
		if (a_event.menuName == RE::Console::MENU_NAME) {
			blockInput = a_event.opening;
		} else if (a_event.menuName == "PauseMenu") {
			if (a_event.opening) {
				if (openFromPauseMenu) {
					openFromPauseMenu = SetupPauseMenu();
				}
			} else {
				Settings::GetSingleton()->LoadMCMSettings();
			}
		}

		return EventResult::kContinue;
	}
}

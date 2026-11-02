#include "PhotoMode/Tabs/Overlays.h"
#include "OverlayFiles.h"

#include "ImGui/Widgets.h"

namespace PhotoMode
{
	void Overlays::LoadOverlays()
	{
        RevertOverlays();
        overlays.clear(); folders.names.clear(); folderFiles.clear(); hasOverlays = false;
        const std::filesystem::path root("Data/Interface/PhotoMode/overlays");
        std::error_code error;
        const auto files = FindOverlayFiles(root, error);
        if (error) logger::error("Overlay discovery at {} failed: {}. Original Photo Mode overlay assets must be installed separately.", std::filesystem::absolute(root).string(), error.message());
        std::uint32_t categoryIndex = 0;
        std::size_t loaded = 0;
        for (const auto& [category, paths] : files) {
            std::vector<std::string> names{ "$NONE"_T };
            for (const auto& path : paths) {
                const auto name = path.stem().string();
                auto [entry, inserted] = overlays[category].try_emplace(name, path.wstring());
                if (inserted && entry->second.Load(true)) {
                    names.push_back(name); ++loaded;
                } else if (inserted) {
                    logger::error("Could not load overlay {}", path.string());
                    overlays[category].erase(entry);
                }
            }
            if (names.size() > 1) {
                folders.names.push_back(category);
                folderFiles[categoryIndex++].names = std::move(names);
            }
        }
        hasOverlays = loaded > 0;
        logger::info("Loaded {} overlays in {} categories from {}", loaded, folders.names.size(), std::filesystem::absolute(root).string());
    }

	void Overlays::RevertOverlays()
	{
		cachedOverlay = nullptr;
		updateOverlay = false;

		folders.index = 0;
		for (auto& [index, files] : folderFiles) {
			files.index = 0;
		}

		alpha = 1.0f;
	}

	ImGui::Texture* Overlays::UpdateOverlay()
	{
		if (const auto it = overlays.find(folders.get_file()); it != overlays.end()) {
			const auto& file = GetFiles().get_file();
			if (const auto fileIt = it->second.find(file); fileIt != it->second.end()) {
				return &fileIt->second;
			}
		}

		return nullptr;
	}

	std::pair<ImGui::Texture*, float> Overlays::GetCurrentOverlay() const
	{
		return { cachedOverlay, alpha };
	}

	void Overlays::Draw()
	{
		ImGui::UpdateWidgetPadding(5, "1.00");

		ImDrawList* drawList = ImGui::GetWindowDrawList();
		drawList->ChannelsSplit(2);

		if (!hasOverlays) {
			ImGui::TextUnformatted("$PM_NoOverlaysInstalled"_T);
		} else {
			ImGui::DisplayWidgetsGroups([&] {
				bool selected, hovered;
				std::tie(selected, hovered) = ImGui::EnumSlider("$PM_Category"_T, &folders.index, folders.names, false);
				if (selected) {
					// reset others to NONE
					for (auto& [index, files] : folderFiles) {
						if (index != folders.index) {
							files.index = 0;
						}
					}
					cachedOverlay = nullptr;
					updateOverlay = false;
					alpha = 1.0f;
				}
				return hovered;
			});
			ImGui::Indent();
			{
				ImGui::DisplayWidgetsGroups([&] {
					bool selected, hovered;
					std::tie(selected, hovered) = ImGui::EnumSlider("$PM_Overlay"_T, &GetFiles().index, GetFiles().names, false);
					if (selected) {
						updateOverlay = true;
						alpha = 1.0f;
					}
					return hovered;
				});
			}
			ImGui::Unindent();
			ImGui::DisplayWidgetsGroups([&] {
				return ImGui::Slider("$PM_Intensity"_T, &alpha, 0.0f, 1.0f).second;
			});

		}
        drawList->ChannelsMerge();
	}

	void Overlays::DrawOverlays()
	{
		const auto drawList = ImGui::GetBackgroundDrawList();
		const auto size = ImGui::GetWindowSize();

		constexpr auto topLeft = ImVec2(0.0f, 0.0f);
		const auto bottomRight = ImVec2(size.x, size.y);

		if (updateOverlay) {
			updateOverlay = false;
			cachedOverlay = UpdateOverlay();
		}

		if (cachedOverlay) {
			drawList->AddImage(cachedOverlay->srView.Get(), topLeft, bottomRight, ImVec2(0, 0), ImVec2(1, 1), static_cast<ImU32>(ImColor(1.0f, 1.0f, 1.0f, alpha)));
		}
	}
}

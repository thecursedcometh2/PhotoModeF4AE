#include "Screenshots/Manager.h"

#include "Graphics.h"
#include "PhotoMode/Manager.h"
#include "Settings.h"

namespace Screenshot
{
	Image::Image(std::string_view a_path, std::uint32_t a_index) :
		index(a_index),
		path(std::format("{}/screenshot{}.dds", a_path, a_index))
	{}

	Image::Image(std::string& a_path) :
		path(Texture::Sanitize(a_path))
	{
		srell::smatch       matches;
		static srell::regex screenshotPattern{ R"(screenshot(\d+))" };
		if (srell::regex_search(path, matches, screenshotPattern)) {
			if (matches.size() > 1) {
				index = string::to_num<std::int32_t>(matches[1].str());
			}
		}
	}
	
	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
		useCustomFolderDirectory = a_ini.GetBoolValue("Screenshots", "bCustomPhotoFolder", useCustomFolderDirectory);
		autoHideMenus = a_ini.GetBoolValue("Screenshots", "bAutoHideMenus", autoHideMenus);
		allowMultiScreenshots = a_ini.GetBoolValue("Screenshots", "bMultiScreenshots", allowMultiScreenshots);
	}

	void Manager::LoadScreenshots()
	{
		logger::info("Loading screenshots...");

		if (auto directory = logger::log_directory()) {
			directory->remove_filename();
			*directory /= "Photos";
			photoDirectory = *directory;
		}

		std::error_code ec;
		if (!std::filesystem::exists(photoDirectory, ec)) {
			logger::info("\tPhoto directory does not exist, creating it... ({})", ec.message());
			std::filesystem::create_directory(photoDirectory);
		}

		logger::info("\tScreenshot directory : {}", photoDirectory.string());

		Settings::GetSingleton()->Save(FileType::kMCM, [this](auto& ini) {
			index = ini.GetLongValue("Screenshots", "iScreenshotIndex", index);
			AssignHighestPossibleIndex();
			ini.SetLongValue("Screenshots", "iScreenshotIndex", index);
		});

		logger::info("\tscreenshot index : {}", index);
	}

	std::uint32_t Manager::GetIndex() const
	{
		return index;
	}

	void Manager::AssignHighestPossibleIndex()
	{
		const auto get_photos_index = [this]() {
			std::vector<Image> photos{};
			for (const auto& entry : std::filesystem::directory_iterator(photoDirectory)) {
				if (entry.is_regular_file()) {
					if (const auto& path = entry.path(); path.extension() == ".png") {
						auto pathStr = entry.path().string();
						photos.push_back(pathStr);
					}
				}
			}
			if (photos.empty()) {
				return -1;
			}
			std::sort(photos.begin(), photos.end());
			return photos.back().index + 1;
		};

		auto mcmIndex = index;
		auto photosIndex = get_photos_index();
		auto vanillaScreenshotIndex = RE::GetINISetting("iScreenShotIndex:Display")->GetInt();

		logger::info("\tAssigning highest screenshot index...");
		logger::info("\t\tmcm index: {}", mcmIndex);
		logger::info("\t\tphoto directory index: {}", photosIndex);
		logger::info("\t\tvanilla directory index: {}", vanillaScreenshotIndex);

		std::set<std::int32_t> indices;
		indices.insert({ mcmIndex, photosIndex, vanillaScreenshotIndex });

		index = *indices.rbegin();
	}

	void Manager::IncrementIndex()
	{
		index++;
		Settings::GetSingleton()->Save(FileType::kMCM, [this](auto& ini) {
			ini.SetLongValue("Screenshots", "iScreenshotIndex", index);
		});
	}

	bool Manager::AllowMultiScreenshots() const
	{
		return allowMultiScreenshots;
	}

	bool Manager::CanAutoHideMenus() const
	{
		return autoHideMenus;
	}

    bool Manager::TakeScreenshot()
    {
        const auto renderer = RE::BSGraphics::GetRendererData();
        if (!renderer || !renderer->device || !renderer->context) return false;
        // This render-target mapping remains game-specific; see RUNTIME_AUDIT.md.
        const auto targets = RE::BSGraphics::RenderTargetManager::GetSingleton();
        if (!targets) return false;
        const auto target = targets->renderTargetID[63];
        if (target >= std::size(renderer->renderTargets)) return false;
        auto* texture = reinterpret_cast<ID3D11Texture2D*>(renderer->renderTargets[target].texture);
        if (!texture) return false;
        DirectX::ScratchImage captured;
        const auto capture = DirectX::CaptureTexture(reinterpret_cast<ID3D11Device*>(renderer->device),
            reinterpret_cast<ID3D11DeviceContext*>(renderer->context), texture, captured);
        if (FAILED(capture)) {
            logger::error("Screenshot readback failed: HRESULT {}", static_cast<unsigned long>(capture));
            return false;
        }
        const auto* source = captured.GetImage(0, 0, 0);
        if (!source) return false;
        DirectX::ScratchImage input;
        const auto converted = Texture::ToRGBA8(*source, input);
        if (FAILED(converted)) return false;
        const std::string path = useCustomFolderDirectory ?
            std::format("{}/Screenshot{}.png", photoDirectory.string(), GetIndex()) :
            std::format("{}_{}.png", RE::GetINISetting("sScreenShotBaseName:Display")->GetString(), GetIndex());
        bool saved = false;
        if (const auto [overlay, alpha] = MANAGER(PhotoMode)->GetOverlay(); overlay) {
            if (!overlay->image) return false;
            const auto* original = overlay->image->GetImage(0, 0, 0);
            if (!original) return false;
            DirectX::ScratchImage rgba;
            const auto convertedOverlay = Texture::ToRGBA8(*original, rgba);
            if (FAILED(convertedOverlay)) return false;
            DirectX::ScratchImage resized;
            const auto* layer = rgba.GetImage(0, 0, 0);
            if (layer->width != source->width || layer->height != source->height) {
                if (FAILED(DirectX::Resize(*layer, source->width, source->height,
                    DirectX::TEX_FILTER_CUBIC, resized))) return false;
                layer = resized.GetImage(0, 0, 0);
            }
            DirectX::ScratchImage blended;
            if (!Texture::AlphaBlendImage(input.GetImage(0, 0, 0), layer, blended, alpha)) return false;
            saved = Texture::SaveToPNG(blended, path);
        } else {
            saved = Texture::SaveToPNG(input, path);
        }
        // Suppress vanilla capture and advance the index only after successful encoding.
        if (saved) IncrementIndex();
        return saved;
    }
}

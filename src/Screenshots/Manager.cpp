#include "Screenshots/Manager.h"
#include "ScreenshotFiles.h"

#include "Graphics.h"
#include "PhotoMode/Manager.h"
#include "Settings.h"

namespace Screenshot
{
	void Manager::LoadMCMSettings(const CSimpleIniA& a_ini)
	{
        if (const auto persisted = a_ini.GetValue("Screenshots", "iScreenshotIndex", nullptr)) {
            if (const auto parsed = ParseIndex(persisted)) index = std::max(index, *parsed);
        }
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
			std::filesystem::create_directories(photoDirectory, ec);
            if (ec) logger::error("Could not create photo directory: {}", ec.message());
		}

		logger::info("\tScreenshot directory : {}", photoDirectory.string());

        // Discovery must run even when neither optional MCM INI exists.
        AssignHighestPossibleIndex();

		logger::info("\tscreenshot index : {}", index);
	}

	std::uint64_t Manager::GetIndex() const
	{
		return index;
	}

    std::filesystem::path Manager::DestinationPrefix() const
    {
        if (useCustomFolderDirectory) return photoDirectory / "Screenshot";
        const auto setting = RE::GetINISetting("sScreenShotBaseName:Display");
        auto base = setting ? setting->GetString() : std::string_view{};
        if (base.empty()) base = "ScreenShot";
        auto prefix = std::filesystem::u8path(base.begin(), base.end());
        prefix += "_";
        return prefix;
    }

    void Manager::AssignHighestPossibleIndex()
    {
        const auto vanilla = RE::GetINISetting("iScreenShotIndex:Display");
        if (vanilla && vanilla->GetInt() > 0) index = std::max(index, static_cast<std::uint64_t>(vanilla->GetInt()));
        const auto prefix = DestinationPrefix();
        std::error_code error;
        if (!prefix.parent_path().empty()) std::filesystem::create_directories(prefix.parent_path(), error);
        if (!error) index = NextFileIndex(prefix, index, error);
        if (error) logger::error("Could not scan screenshot destination {}: {}. Exclusive creation still prevents replacement.", prefix.string(), error.message());
    }

    void Manager::IncrementIndex()
    {
        if (index != std::numeric_limits<std::uint64_t>::max()) ++index;
        // Recover from filenames on restart. MCM's i* settings are signed 32-bit;
        // never write our 64-bit bookkeeping into its user settings store.
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
        const std::scoped_lock lock(screenshotMutex);
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
        const auto prefix = DestinationPrefix();
        std::filesystem::path savedPath;
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
            saved = Texture::SaveNewPNG(blended, prefix, index, savedPath);
        } else {
            saved = Texture::SaveNewPNG(input, prefix, index, savedPath);
        }
        // Suppress vanilla capture and advance the index only after successful encoding.
        if (saved) {
            logger::info("Saved screenshot {}", savedPath.string());
            IncrementIndex();
        }
        return saved;
    }
}

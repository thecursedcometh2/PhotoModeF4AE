#include "Graphics.h"
#include "ImageMath.h"

namespace Texture
{
	std::string Sanitize(std::string& a_path)
	{
		a_path = clib_util::string::tolower(a_path);

		a_path = srell::regex_replace(a_path, srell::regex("/+|\\\\+"), "\\");
		a_path = srell::regex_replace(a_path, srell::regex("^\\\\+"), "");
		a_path = srell::regex_replace(a_path, srell::regex(R"(.*?[^\s]textures\\|^textures\\)", srell::regex::icase), "");

		return a_path;
	}

    HRESULT ToRGBA8(const DirectX::Image& source, DirectX::ScratchImage& output)
    {
        // Treat sRGB SDR texels as encoded bytes during channel conversion.
        // Otherwise Convert can decode sRGB and WIC FORCE_SRGB merely retags it,
        // producing a dark PNG. No transfer change is needed for SDR capture.
        auto view = source;
        view.format = DirectX::MakeLinear(view.format);
        if (view.format == DXGI_FORMAT_R8G8B8A8_UNORM) {
            return output.InitializeFromImage(view);
        }
        const auto filter = DirectX::IsSRGB(source.format) ||
            source.format == DXGI_FORMAT_B8G8R8A8_UNORM ||
            source.format == DXGI_FORMAT_B8G8R8X8_UNORM ?
            DirectX::TEX_FILTER_DEFAULT : DirectX::TEX_FILTER_SRGB_OUT;
        return DirectX::Convert(view, DXGI_FORMAT_R8G8B8A8_UNORM,
            filter, DirectX::TEX_THRESHOLD_DEFAULT, output);
    }

    bool AlphaBlendImage(const DirectX::Image* base, const DirectX::Image* overlay,
        DirectX::ScratchImage& output, float intensity)
    {
        if (!base || !overlay || base->width != overlay->width || base->height != overlay->height ||
            base->format != DXGI_FORMAT_R8G8B8A8_UNORM || overlay->format != base->format) {
            return false;
        }
        if (FAILED(output.InitializeFromImage(*base))) return false;
        const auto* result = output.GetImage(0, 0, 0);
        return ImageMath::CompositeOpaque(result->pixels, result->rowPitch,
            overlay->pixels, overlay->rowPitch, base->width, base->height, intensity);
    }

    bool SaveToPNG(const DirectX::ScratchImage& input, std::string_view path)
    {
        const auto* source = input.GetImage(0, 0, 0);
        if (!source) return false;
        DirectX::ScratchImage rgba;
        HRESULT hr = ToRGBA8(*source, rgba);
        if (FAILED(hr)) return false;
        const auto* image = rgba.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < image->height; ++y) {
            auto* row = image->pixels + y * image->rowPitch;
            for (std::size_t x = 0; x < image->width; ++x) row[x * 4 + 3] = 255;
        }
        const auto wide = stl::utf8_to_utf16(path);
        if (!wide) return false;
        hr = DirectX::SaveToWICFile(*image, DirectX::WIC_FLAGS_FORCE_SRGB,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), wide->c_str());
        if (FAILED(hr)) logger::error("Failed to save PNG {}: HRESULT {}", path, static_cast<unsigned long>(hr));
        return SUCCEEDED(hr);
    }
}

#include "Graphics.h"
#include "ImageMath.h"
#include "ScreenshotFiles.h"

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

    bool SaveNewPNG(const DirectX::ScratchImage& input, const std::filesystem::path& prefix,
        std::uint64_t& index, std::filesystem::path& savedPath)
    {
        savedPath.clear();
        const auto* source = input.GetImage(0, 0, 0);
        if (!source) return false;
        DirectX::ScratchImage rgba;
        if (FAILED(ToRGBA8(*source, rgba))) return false;
        const auto* image = rgba.GetImage(0, 0, 0);
        for (std::size_t y = 0; y < image->height; ++y) {
            auto* row = image->pixels + y * image->rowPitch;
            for (std::size_t x = 0; x < image->width; ++x) row[x * 4 + 3] = 255;
        }
        DirectX::Blob encoded;
        const auto encodedResult = DirectX::SaveToWICMemory(*image, DirectX::WIC_FLAGS_FORCE_SRGB,
            DirectX::GetWICCodec(DirectX::WIC_CODEC_PNG), encoded);
        if (FAILED(encodedResult)) {
            logger::error("Could not encode screenshot: HRESULT {}", static_cast<unsigned long>(encodedResult));
            return false;
        }
        for (;;) {
            const auto path = Screenshot::NumberedPath(prefix, index);
            // CREATE_NEW atomically refuses existing files, including concurrent
            // shots and files added after the startup directory scan.
            const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE | DELETE, 0, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (file == INVALID_HANDLE_VALUE) {
                const auto error = GetLastError();
                if (error == ERROR_FILE_EXISTS || error == ERROR_ALREADY_EXISTS) {
                    if (index == std::numeric_limits<std::uint64_t>::max()) {
                        logger::error("Screenshot numbering exhausted; existing photo preserved");
                        return false;
                    }
                    ++index;
                    continue;
                }
                logger::error("Could not create screenshot {}: Windows error {}", path.string(), error);
                return false;
            }
            bool success = true;
            std::size_t offset = 0;
            while (offset < encoded.GetBufferSize()) {
                const auto chunk = static_cast<DWORD>(std::min<std::size_t>(encoded.GetBufferSize() - offset, MAXDWORD));
                DWORD written = 0;
                if (!WriteFile(file, encoded.GetBufferPointer() + offset, chunk, &written, nullptr) || written == 0) {
                    success = false; break;
                }
                offset += written;
            }
            if (success && !FlushFileBuffers(file)) success = false;
            if (!success) {
                // Delete only the file we own, through its exclusive handle.
                FILE_DISPOSITION_INFO disposition{ TRUE };
                SetFileInformationByHandle(file, FileDispositionInfo, &disposition, sizeof(disposition));
            }
            if (!CloseHandle(file)) success = false;
            if (!success) {
                logger::error("Screenshot write failed for {}", path.string());
                return false;
            }
            savedPath = path;
            return true;
        }
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

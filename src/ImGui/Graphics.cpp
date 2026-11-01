#include "ImGui/Graphics.h"

#include "ImGui/Renderer.h"

namespace ImGui
{
	Texture::Texture(std::wstring_view a_path) :
		path(a_path)
	{}

	Texture::Texture(std::wstring_view a_folder, std::wstring_view a_textureName)
	{
		path.append(a_folder).append(a_textureName).append(L".png");
	}

	Texture::~Texture()
	{
		if (srView) {
			srView.Reset();
		}
		if (image) {
			image.reset();
		}
	}

	bool Texture::Load(bool a_resizeToScreenRes)
	{
		bool result = false;

		image = std::make_shared<DirectX::ScratchImage>();
		HRESULT hr = DirectX::LoadFromWICFile(path.c_str(), DirectX::WIC_FLAGS_IGNORE_SRGB, nullptr, *image);

		if (SUCCEEDED(hr)) {
			if (auto renderer = RE::BSGraphics::GetRendererData()) {
				if (a_resizeToScreenRes) {
					const auto state = RE::BSGraphics::State::GetSingleton();
                    if (!state) return false;
                    const std::pair<std::size_t, std::size_t> screenSize = { state->backBufferWidth, state->backBufferHeight };
					if (screenSize.second != image->GetMetadata().height || screenSize.first != image->GetMetadata().width) {
						DirectX::ScratchImage tmpImage;
						if (FAILED(DirectX::Resize(*image->GetImage(0, 0, 0), screenSize.first, screenSize.second, DirectX::TEX_FILTER_CUBIC, tmpImage))) return false;

						image.reset();  // is this needed
						image = std::make_shared<DirectX::ScratchImage>(std::move(tmpImage));
					}
				}

				ComPtr<ID3D11Resource> pTexture{};
				hr = DirectX::CreateTexture((ID3D11Device*)renderer->device, image->GetImages(), 1, image->GetMetadata(), &pTexture);

				if (SUCCEEDED(hr)) {
					D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc{};
					srvDesc.Format = image->GetMetadata().format;
					srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
					srvDesc.Texture2D.MipLevels = 1;
					srvDesc.Texture2D.MostDetailedMip = 0;

					hr = ((ID3D11Device*)renderer->device)->CreateShaderResourceView(pTexture.Get(), &srvDesc, &srView);
					result = SUCCEEDED(hr);
				}

				size.x = static_cast<float>(image->GetMetadata().width);
				size.y = static_cast<float>(image->GetMetadata().height);

				pTexture.Reset();
			}
		}

		return result;
	}
}

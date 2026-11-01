#include "ImGui/Renderer.h"
#include "ImGui/Fonts/Manager.h"
#include "ImGui/Styles.h"

#include "PhotoMode/Manager.h"

namespace ImGui::Renderer
{
	float GetResolutionScale()
	{
		const auto state = RE::BSGraphics::State::GetSingleton();
        return state ? state->backBufferHeight / 1080.0f : 1.0f;
	}
	
	struct WndProc
	{
		static LRESULT thunk(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
		{
			auto& io = ImGui::GetIO();
			if (uMsg == WM_KILLFOCUS) {
				io.ClearInputKeys();
			}

			return func(hWnd, uMsg, wParam, lParam);
		}
		static inline WNDPROC func;
	};
	
	void Init()
	{
		if (const auto renderer = RE::BSGraphics::GetRendererData()) {
			const auto swapChain = (IDXGISwapChain*)renderer->renderWindow[0].swapChain;
			if (!swapChain) {
				logger::error("couldn't find swapChain");
				return;
			}

			DXGI_SWAP_CHAIN_DESC desc{};
			if (FAILED(swapChain->GetDesc(std::addressof(desc)))) {
				logger::error("IDXGISwapChain::GetDesc failed.");
				return;
			}

			const auto device = (ID3D11Device*)renderer->device;
			const auto context = (ID3D11DeviceContext*)renderer->context;

			logger::info("Initializing ImGui..."sv);

			ImGui::CreateContext();

			auto& io = ImGui::GetIO();
			io.IniFilename = nullptr;

			if (!F4SE::ImGui_ImplWin32_Init(desc.OutputWindow)) {
				logger::error("ImGui initialization failed (Win32)");
				return;
			}
			if (!ImGui_ImplDX11_Init(device, context)) {
				logger::error("ImGui initialization failed (DX11)"sv);
				return;
			}

			RECT rect = { 0, 0, 0, 0 };
			GetClientRect((HWND)renderer->renderWindow[0].hwnd, &rect);
			ImGui::GetIO().DisplaySize = ImVec2((float)(rect.right - rect.left), (float)(rect.bottom - rect.top));

			logger::info("ImGui initialized.");

			initialized.store(true);

			WndProc::func = reinterpret_cast<WNDPROC>(
				SetWindowLongPtrA(
					desc.OutputWindow,
					GWLP_WNDPROC,
					reinterpret_cast<LONG_PTR>(WndProc::thunk)));
			if (!WndProc::func) {
				logger::error("SetWindowLongPtrA failed!");
			}
		}
	}

	//
	struct PostDisplay
	{
		static void thunk(RE::IMenu* a_menu)
		{
			if (initialized.load()) {
				const auto photoMode = MANAGER(PhotoMode);

				if (!photoMode->IsActive() || !photoMode->OnFrameUpdate()) {
					return func(a_menu);
				}

				ImGui_ImplDX11_NewFrame();
				F4SE::ImGui_ImplWin32_NewFrame();
				ImGui::NewFrame();
				{
					// disable windowing
					GImGui->NavWindowingTarget = nullptr;

					photoMode->Draw();
				}
				ImGui::EndFrame();
				ImGui::Render();
				ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
			}

			return func(a_menu);
		}
		static inline REL::Relocation<decltype(thunk)> func;
		static inline std::size_t                      idx{ 0x6 };
	};

	void Install()
	{
		//REL::Relocation<std::uintptr_t> target{ REL::ID(2276814), 0x31D };  // BSGraphics::InitD3D
		//stl::write_thunk_call<CreateD3DAndSwapChain>(target.address());

		stl::write_vfunc<RE::HUDMenu, PostDisplay>();
	}
}

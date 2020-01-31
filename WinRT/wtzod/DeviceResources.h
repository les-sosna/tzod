#pragma once
#include <wrl/client.h>

struct ID3D11Device2;
struct ID3D11DeviceContext2;

struct IRender;
class RenderBinding;
class SwapChainResources;

namespace DX
{
	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		explicit DeviceResources(Windows::UI::Core::CoreWindow^ coreWindow);
		~DeviceResources();
		bool ValidateDevice() const;
		bool IsDeviceRemoved() const;
		void Present();

		IRender& GetRender(int rotationAngle, int width, int height);
		RenderBinding& GetRenderBinding() { return *_renderBinding; }

	private:
		Microsoft::WRL::ComPtr<ID3D11Device2> _d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext2> _d3dContext;

		std::unique_ptr<SwapChainResources> _swapChainResources;
		std::unique_ptr<IRender> _render;
		std::unique_ptr<RenderBinding> _renderBinding;
	};
}
#pragma once
#include <wrl/client.h>

struct ID3D11Device2;
struct ID3D11DeviceContext2;

struct IRender;
class RenderBinding;

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
	Microsoft::WRL::ComPtr<IDXGISwapChain3> _swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> _d3dRenderTargetView;

	int m_pxWidth = 0;
	int m_pxHeight = 0;
	int m_rotationAngle = -1;

	std::unique_ptr<IRender> _render;
	std::unique_ptr<RenderBinding> _renderBinding;

	HRESULT ResizeSwapChainInternal(int width, int height, int rotationAngle);
};

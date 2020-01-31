#pragma once
#include <wrl/client.h>

struct IDXGISwapChain3;
struct ID3D11RenderTargetView;
struct ID3D11Device;
struct ID3D11DeviceContext2;

class SwapChainResources
{
public:
	explicit SwapChainResources(IDXGISwapChain3 *swapChain);
	~SwapChainResources();

	HRESULT SetPixelSize(ID3D11Device *device, ID3D11DeviceContext2 *deviceContext, int width, int height);
	HRESULT SetCurrentOrientation(ID3D11Device *device, ID3D11DeviceContext2 *deviceContext, int currentOrientation);

	IDXGISwapChain3* GetSwapChain() const { return m_swapChain.Get(); }
	ID3D11RenderTargetView* GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }

private:
	HRESULT ResizeSwapChainInternal(ID3D11Device *device, ID3D11DeviceContext2 *deviceContext, int width, int height, int rotationAngle);

	Microsoft::WRL::ComPtr<IDXGISwapChain3> m_swapChain;
	Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;

	int m_pxWidth = 0;
	int m_pxHeight = 0;
	int m_rotationAngle = -1;
};

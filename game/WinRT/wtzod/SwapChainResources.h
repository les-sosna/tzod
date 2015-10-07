#pragma once

namespace DX
{
	class DeviceResources;

	class SwapChainResources
	{
	public:
		SwapChainResources(DeviceResources &deviceResources, Windows::UI::Core::CoreWindow ^coreWindow);
		~SwapChainResources();

		void SetLogicalSize(Windows::Foundation::Size logicalSize);
		void SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		void SetDpi(float dpi, Windows::Foundation::Size logicalSize);

		void Present();


		Windows::Foundation::Size GetOutputSize() const { return m_outputSize; }
		Windows::Foundation::Size GetLogicalSize() const { return m_logicalSize; }

		IDXGISwapChain1*		GetSwapChain() const { return m_swapChain.Get(); }
		ID3D11RenderTargetView*	GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }
		ID3D11DepthStencilView* GetDepthStencilView() const { return m_d3dDepthStencilView.Get(); }
		DirectX::XMFLOAT4X4		GetOrientationTransform3D() const { return m_orientationTransform3D; }
		Windows::Foundation::Size GetRenderTargetSize() const { return m_d3dRenderTargetSize; }

	private:
		void OnResize();
		DXGI_MODE_ROTATION ComputeDisplayRotation();

		DeviceResources &m_deviceResources;

		Microsoft::WRL::ComPtr<IDXGISwapChain1>			m_swapChain;

		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_d3dRenderTargetView;
		Microsoft::WRL::ComPtr<ID3D11DepthStencilView>	m_d3dDepthStencilView;

		Windows::Foundation::Size						m_outputSize;
		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;

		Windows::Foundation::Size						m_d3dRenderTargetSize;
		DirectX::XMFLOAT4X4								m_orientationTransform3D;
	};
}

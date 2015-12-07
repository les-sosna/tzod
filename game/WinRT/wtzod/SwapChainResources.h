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
		void SetDpi(float dpi);

		IDXGISwapChain1*		GetSwapChain() const { return m_swapChain.Get(); }
		ID3D11RenderTargetView*	GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }

	private:
		void ResizeSwapChain(Windows::Foundation::Size logicalSize, float dpi);

		DeviceResources &m_deviceResources;

		Microsoft::WRL::ComPtr<IDXGISwapChain1>			m_swapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView>	m_d3dRenderTargetView;

		Windows::Foundation::Size						m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations	m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations	m_currentOrientation;
		float											m_dpi;
	};
}

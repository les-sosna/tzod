#pragma once

namespace DX
{
	class DeviceResources;

	class SwapChainResources
	{
	public:
		SwapChainResources(DeviceResources &deviceResources, Windows::UI::Core::CoreWindow ^coreWindow);
		~SwapChainResources();

		bool SetLogicalSize(Windows::Foundation::Size logicalSize);
		bool SetCurrentOrientation(Windows::Graphics::Display::DisplayOrientations currentOrientation);
		bool SetDpi(float dpi);

		IDXGISwapChain1* GetSwapChain() const { return m_swapChain.Get(); }
		ID3D11RenderTargetView* GetBackBufferRenderTargetView() const { return m_d3dRenderTargetView.Get(); }

	private:
		bool ResizeSwapChain(Windows::Foundation::Size logicalSize, float dpi);

		DeviceResources &m_deviceResources;

		Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swapChain;
		Microsoft::WRL::ComPtr<ID3D11RenderTargetView> m_d3dRenderTargetView;

		Windows::Foundation::Size m_logicalSize;
		Windows::Graphics::Display::DisplayOrientations m_nativeOrientation;
		Windows::Graphics::Display::DisplayOrientations m_currentOrientation;
		float m_dpi;
	};
}

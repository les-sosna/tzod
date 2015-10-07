#include "pch.h"
#include "DeviceResources.h"
#include "SwapChainResources.h"
#include "DirectXHelper.h"

using namespace DirectX;
using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;

// Constants used to calculate screen rotations
namespace ScreenRotation
{
	// 0-degree Z-rotation
	static const XMFLOAT4X4 Rotation0(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 90-degree Z-rotation
	static const XMFLOAT4X4 Rotation90(
		0.0f, 1.0f, 0.0f, 0.0f,
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 180-degree Z-rotation
	static const XMFLOAT4X4 Rotation180(
		-1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, -1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);

	// 270-degree Z-rotation
	static const XMFLOAT4X4 Rotation270(
		0.0f, -1.0f, 0.0f, 0.0f,
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f
		);
};

DX::SwapChainResources::SwapChainResources(DeviceResources &deviceResources, CoreWindow ^coreWindow)
	: m_deviceResources(deviceResources)
	, m_d3dRenderTargetSize()
	, m_outputSize()
	, m_logicalSize(coreWindow->Bounds.Width, coreWindow->Bounds.Height)
	, m_nativeOrientation(DisplayOrientations::None)
	, m_currentOrientation(DisplayOrientations::None)
	, m_dpi(-1.0f)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_dpi = currentDisplayInformation->LogicalDpi;


	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };

	swapChainDesc.Width = lround(m_d3dRenderTargetSize.Width); // Match the size of the window.
	swapChainDesc.Height = lround(m_d3dRenderTargetSize.Height);
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	swapChainDesc.Flags = 0;
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;

	// This sequence obtains the DXGI factory that was used to create the Direct3D device.
	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(
		m_deviceResources.GetD3DDevice()->QueryInterface(IID_PPV_ARGS(&dxgiDevice))
		);

	ComPtr<IDXGIAdapter> dxgiAdapter;
	DX::ThrowIfFailed(
		dxgiDevice->GetAdapter(&dxgiAdapter)
		);

	ComPtr<IDXGIFactory2> dxgiFactory;
	DX::ThrowIfFailed(
		dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory))
		);

	DX::ThrowIfFailed(
		dxgiFactory->CreateSwapChainForCoreWindow(
			m_deviceResources.GetD3DDevice(),
			reinterpret_cast<IUnknown*>(coreWindow),
			&swapChainDesc,
			nullptr,
			&m_swapChain
			)
		);

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	DX::ThrowIfFailed(
		dxgiDevice->SetMaximumFrameLatency(1)
		);

	OnResize();
}

DX::SwapChainResources::~SwapChainResources()
{
}

// This method is called in the event handler for the SizeChanged event.
void DX::SwapChainResources::SetLogicalSize(Windows::Foundation::Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize = logicalSize;
		OnResize();
	}
}

// This method is called in the event handler for the DpiChanged event.
void DX::SwapChainResources::SetDpi(float dpi, Size logicalSize)
{
	if (dpi != m_dpi || m_logicalSize != logicalSize)
	{
		m_dpi = dpi;
		m_logicalSize = logicalSize; 
		OnResize();
	}
}

// This method is called in the event handler for the OrientationChanged event.
void DX::SwapChainResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		OnResize();
	}
}

// Present the contents of the swap chain to the screen.
void DX::SwapChainResources::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = m_swapChain->Present(1, 0);

	
	// Discard the contents of the render target.
	// This is a valid operation only when the existing contents will be entirely
	// overwritten. If dirty or scroll rects are used, this call should be removed.
	m_deviceResources.GetD3DDeviceContext()->DiscardView(m_d3dRenderTargetView.Get());

	// Discard the contents of the depth stencil.
	m_deviceResources.GetD3DDeviceContext()->DiscardView(m_d3dDepthStencilView.Get());
	

	// If the device was removed either by a disconnection or a driver upgrade, we 
	// must recreate all device resources.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		assert(false);
		//HandleDeviceLost();
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}
}

void DX::SwapChainResources::OnResize()
{
	assert(m_swapChain != nullptr);

	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_deviceResources.GetD3DDeviceContext()->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_d3dDepthStencilView = nullptr;
	m_deviceResources.GetD3DDeviceContext()->Flush();

	// Calculate the necessary render target size in pixels.
	m_outputSize.Width = DX::ConvertDipsToPixels(m_logicalSize.Width, m_dpi);
	m_outputSize.Height = DX::ConvertDipsToPixels(m_logicalSize.Height, m_dpi);

	// Prevent zero size DirectX content from being created.
	m_outputSize.Width = max(m_outputSize.Width, 1);
	m_outputSize.Height = max(m_outputSize.Height, 1);

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	DXGI_MODE_ROTATION displayRotation = ComputeDisplayRotation();

	bool swapDimensions = displayRotation == DXGI_MODE_ROTATION_ROTATE90 || displayRotation == DXGI_MODE_ROTATION_ROTATE270;
	m_d3dRenderTargetSize.Width = swapDimensions ? m_outputSize.Height : m_outputSize.Width;
	m_d3dRenderTargetSize.Height = swapDimensions ? m_outputSize.Width : m_outputSize.Height;


	HRESULT hr = m_swapChain->ResizeBuffers(
		2, // Double-buffered swap chain.
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		DXGI_FORMAT_B8G8R8A8_UNORM,
		0
		);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		// If the device was removed for any reason, a new device and swap chain will need to be created.
		assert(false);
		//HandleDeviceLost();

		// Everything is set up now. Do not continue execution of this method. HandleDeviceLost will reenter this method 
		// and correctly set up the new device.
		return;
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}



	// Set the proper orientation for the swap chain, and generate
	// 3D matrix transformation for rendering to the rotated swap chain.
	// The 3D matrix is specified explicitly to avoid rounding errors.

	switch (displayRotation)
	{
	case DXGI_MODE_ROTATION_IDENTITY:
		m_orientationTransform3D = ScreenRotation::Rotation0;
		break;

	case DXGI_MODE_ROTATION_ROTATE90:
		m_orientationTransform3D = ScreenRotation::Rotation270;
		break;

	case DXGI_MODE_ROTATION_ROTATE180:
		m_orientationTransform3D = ScreenRotation::Rotation180;
		break;

	case DXGI_MODE_ROTATION_ROTATE270:
		m_orientationTransform3D = ScreenRotation::Rotation90;
		break;

	default:
		throw ref new FailureException();
	}

	DX::ThrowIfFailed(
		m_swapChain->SetRotation(displayRotation)
		);

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(
		m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer))
		);

	DX::ThrowIfFailed(
		m_deviceResources.GetD3DDevice()->CreateRenderTargetView(
			backBuffer.Get(),
			nullptr,
			&m_d3dRenderTargetView
			)
		);

	// Create a depth stencil view for use with 3D rendering if needed.
	CD3D11_TEXTURE2D_DESC depthStencilDesc(
		DXGI_FORMAT_D24_UNORM_S8_UINT,
		lround(m_d3dRenderTargetSize.Width),
		lround(m_d3dRenderTargetSize.Height),
		1, // This depth stencil view has only one texture.
		1, // Use a single mipmap level.
		D3D11_BIND_DEPTH_STENCIL
		);

	ComPtr<ID3D11Texture2D> depthStencil;
	DX::ThrowIfFailed(
		m_deviceResources.GetD3DDevice()->CreateTexture2D(
			&depthStencilDesc,
			nullptr,
			&depthStencil
			)
		);

	CD3D11_DEPTH_STENCIL_VIEW_DESC depthStencilViewDesc(D3D11_DSV_DIMENSION_TEXTURE2D);
	DX::ThrowIfFailed(
		m_deviceResources.GetD3DDevice()->CreateDepthStencilView(
			depthStencil.Get(),
			&depthStencilViewDesc,
			&m_d3dDepthStencilView
			)
		);
}

// This method determines the rotation between the display device's native Orientation and the
// current display orientation.
DXGI_MODE_ROTATION DX::SwapChainResources::ComputeDisplayRotation()
{
	DXGI_MODE_ROTATION rotation = DXGI_MODE_ROTATION_UNSPECIFIED;

	// Note: NativeOrientation can only be Landscape or Portrait even though
	// the DisplayOrientations enum has other values.
	switch (m_nativeOrientation)
	{
	case DisplayOrientations::Landscape:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;
		}
		break;

	case DisplayOrientations::Portrait:
		switch (m_currentOrientation)
		{
		case DisplayOrientations::Landscape:
			rotation = DXGI_MODE_ROTATION_ROTATE90;
			break;

		case DisplayOrientations::Portrait:
			rotation = DXGI_MODE_ROTATION_IDENTITY;
			break;

		case DisplayOrientations::LandscapeFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE270;
			break;

		case DisplayOrientations::PortraitFlipped:
			rotation = DXGI_MODE_ROTATION_ROTATE180;
			break;
		}
		break;
	}
	return rotation;
}



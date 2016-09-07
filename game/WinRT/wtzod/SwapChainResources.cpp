#include "pch.h"
#include "DeviceResources.h"
#include "SwapChainResources.h"
#include "DirectXHelper.h"
#include "DisplayOrientation.h"

using namespace DirectX;
using namespace Platform;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::Graphics::Display;
using namespace Windows::UI::Core;

DX::SwapChainResources::SwapChainResources(DeviceResources &deviceResources, CoreWindow ^coreWindow)
	: m_deviceResources(deviceResources)
	, m_logicalSize(coreWindow->Bounds.Width, coreWindow->Bounds.Height)
	, m_nativeOrientation(DisplayOrientations::None)
	, m_currentOrientation(DisplayOrientations::None)
	, m_dpi(-1.0f)
{
	DisplayInformation^ currentDisplayInformation = DisplayInformation::GetForCurrentView();

	m_nativeOrientation = currentDisplayInformation->NativeOrientation;
	m_currentOrientation = currentDisplayInformation->CurrentOrientation;
	m_dpi = currentDisplayInformation->LogicalDpi;

	// This sequence obtains the DXGI factory that was used to create the Direct3D device.
	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(m_deviceResources.GetD3DDevice()->QueryInterface(IID_PPV_ARGS(&dxgiDevice)));

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	DX::ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));

	ComPtr<IDXGIAdapter> dxgiAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&dxgiAdapter));

	ComPtr<IDXGIFactory2> dxgiFactory;
	DX::ThrowIfFailed(dxgiAdapter->GetParent(IID_PPV_ARGS(&dxgiFactory)));

	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = 0; // todo: Match the size of the window.
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // This is the most common swap chain format.
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1; // Don't use multi-sampling.
	swapChainDesc.SampleDesc.Quality = 0;
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = 2; // Use double-buffering to minimize latency.
	swapChainDesc.Scaling = DXGI_SCALING_NONE;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_SEQUENTIAL; // All Windows Store apps must use this SwapEffect.
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	swapChainDesc.Flags = 0;

	DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
		m_deviceResources.GetD3DDevice(),
		reinterpret_cast<IUnknown*>(coreWindow),
		&swapChainDesc,
		nullptr,
		&m_swapChain));

	ResizeSwapChain(m_logicalSize, m_dpi);
}

DX::SwapChainResources::~SwapChainResources()
{
}

bool DX::SwapChainResources::SetLogicalSize(Size logicalSize)
{
	if (m_logicalSize != logicalSize)
	{
		m_logicalSize == logicalSize;
		return ResizeSwapChain(logicalSize, m_dpi);
	}
	return true;
}

bool DX::SwapChainResources::SetDpi(float dpi)
{
	if (dpi != m_dpi)
	{
		m_dpi = dpi;
		return ResizeSwapChain(m_logicalSize, dpi);
	}
	return true;
}

// This method is called in the event handler for the OrientationChanged event.
bool DX::SwapChainResources::SetCurrentOrientation(DisplayOrientations currentOrientation)
{
	if (m_currentOrientation != currentOrientation)
	{
		m_currentOrientation = currentOrientation;
		return ResizeSwapChain(m_logicalSize, m_dpi);
	}
	return true;
}

static DXGI_MODE_ROTATION AsDXGIModeRotation(int rotationAngle)
{
	switch (rotationAngle)
	{
	default: assert(0);
	case 0: return DXGI_MODE_ROTATION_IDENTITY;
	case 90: return DXGI_MODE_ROTATION_ROTATE90;
	case 180: return DXGI_MODE_ROTATION_ROTATE180;
	case 270: return DXGI_MODE_ROTATION_ROTATE270;
	}
}

bool DX::SwapChainResources::ResizeSwapChain(Windows::Foundation::Size logicalSize, float dpi)
{
	assert(m_swapChain != nullptr);

	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	m_deviceResources.GetD3DDeviceContext()->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView = nullptr;
	m_deviceResources.GetD3DDeviceContext()->Flush();

	// Calculate the necessary render target size in pixels.
	// Prevent zero size DirectX content from being created.
	Windows::Foundation::Size outputSize;
	outputSize.Width = std::max(std::floor(DX::ConvertDipsToPixels(logicalSize.Width, dpi)), 1.f);
	outputSize.Height = std::max(std::floor(DX::ConvertDipsToPixels(logicalSize.Height, dpi)), 1.f);

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	int rotationAngle = ComputeDisplayRotation(m_nativeOrientation, m_currentOrientation);
	bool swapDimensions = 90 == rotationAngle || 270 == rotationAngle;
	float renderTargetWidth = swapDimensions ? outputSize.Height : outputSize.Width;
	float renderTargetHeight = swapDimensions ? outputSize.Width : outputSize.Height;

	HRESULT hr = m_swapChain->ResizeBuffers(
		2, // Double-buffered swap chain.
		lround(renderTargetWidth),
		lround(renderTargetHeight),
		DXGI_FORMAT_B8G8R8A8_UNORM,
		0);

	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		// If the device was removed for any reason, a new device and swap chain will need to be created.
		return false;
	}
	else
	{
		DX::ThrowIfFailed(hr);
	}

	DX::ThrowIfFailed(m_swapChain->SetRotation(AsDXGIModeRotation(rotationAngle)));

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	DX::ThrowIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

	DX::ThrowIfFailed(m_deviceResources.GetD3DDevice()->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&m_d3dRenderTargetView));

	return true;
}

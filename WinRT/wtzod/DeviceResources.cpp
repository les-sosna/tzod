#include "pch.h"
#include "DeviceResources.h"
#include "DirectXHelper.h"
#include <video/RenderD3D11.h>
#include <video/RenderBinding.h>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Platform;

#define ReturnIfFailed(expr) if(HRESULT hr = (expr); SUCCEEDED(hr)) {} else return hr

static ComPtr<IDXGISwapChain3> CreateSwapchainForCoreWindow(IDXGIDevice* dxgiDevice, IUnknown* deviceForSwapChain, CoreWindow^ coreWindow)
{
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

	ComPtr<IDXGISwapChain1> swapChain;
	DX::ThrowIfFailed(dxgiFactory->CreateSwapChainForCoreWindow(
		deviceForSwapChain,
		reinterpret_cast<IUnknown*>(coreWindow),
		&swapChainDesc,
		nullptr,
		swapChain.ReleaseAndGetAddressOf()));

	ComPtr<IDXGISwapChain3> swapChain3;
	DX::ThrowIfFailed(swapChain.As(&swapChain3));

	return swapChain3;
}

DeviceResources::DeviceResources(CoreWindow^ coreWindow)
	: _renderBinding(new RenderBinding())
{
	// This flag adds support for surfaces with a different color channel ordering
	// than the API default. It is required for compatibility with Direct2D.
	UINT creationFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;

#if defined(_DEBUG)
	if (DX::SdkLayersAvailable())
	{
		// If the project is in a debug build, enable debugging via SDK Layers with this flag.
		creationFlags |= D3D11_CREATE_DEVICE_DEBUG;
	}
#endif

	// This array defines the set of DirectX hardware feature levels this app will support.
	// Note the ordering should be preserved.
	// Don't forget to declare your application's minimum required feature level in its
	// description.  All applications are assumed to support 9.1 unless otherwise stated.
	D3D_FEATURE_LEVEL featureLevels[] =
	{
		D3D_FEATURE_LEVEL_11_1,
		D3D_FEATURE_LEVEL_11_0,
		D3D_FEATURE_LEVEL_10_1,
		D3D_FEATURE_LEVEL_10_0,
		D3D_FEATURE_LEVEL_9_3,
		D3D_FEATURE_LEVEL_9_2,
		D3D_FEATURE_LEVEL_9_1
	};

	// Create the Direct3D 11 API device object and a corresponding context.
	ComPtr<ID3D11Device> device;
	ComPtr<ID3D11DeviceContext> context;

	D3D_FEATURE_LEVEL d3dFeatureLevel = D3D_FEATURE_LEVEL_9_1;

	HRESULT hr = D3D11CreateDevice(
		nullptr,					// Specify nullptr to use the default adapter.
		D3D_DRIVER_TYPE_HARDWARE,	// Create a device using the hardware graphics driver.
		0,							// Should be 0 unless the driver is D3D_DRIVER_TYPE_SOFTWARE.
		creationFlags,				// Set debug and Direct2D compatibility flags.
		featureLevels,				// List of feature levels this app can support.
		ARRAYSIZE(featureLevels),	// Size of the list above.
		D3D11_SDK_VERSION,			// Always set this to D3D11_SDK_VERSION for Windows Store apps.
		&device,					// Returns the Direct3D device created.
		&d3dFeatureLevel,			// Returns feature level of device created.
		&context					// Returns the device immediate context.
		);

	if (FAILED(hr))
	{
		// If the initialization fails, fall back to the WARP device.
		// For more information on WARP, see: 
		// http://go.microsoft.com/fwlink/?LinkId=286690
		DX::ThrowIfFailed(
			D3D11CreateDevice(
				nullptr,
				D3D_DRIVER_TYPE_WARP, // Create a WARP device instead of a hardware device.
				0,
				creationFlags,
				featureLevels,
				ARRAYSIZE(featureLevels),
				D3D11_SDK_VERSION,
				&device,
				&d3dFeatureLevel,
				&context
				)
			);
	}

	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(device.As(&dxgiDevice));

	// Ensure that DXGI does not queue more than one frame at a time. This both reduces latency and
	// ensures that the application will only render after each VSync, minimizing power consumption.
	DX::ThrowIfFailed(dxgiDevice->SetMaximumFrameLatency(1));


	// Store pointers to the Direct3D 11.1 API device and immediate context.
	DX::ThrowIfFailed(device.As(&_d3dDevice));
	DX::ThrowIfFailed(context.As(&_d3dContext));

	_swapChain = CreateSwapchainForCoreWindow(dxgiDevice.Get(), _d3dDevice.Get(), coreWindow);
	_render.reset(new RenderD3D11(*_d3dRenderTargetView.GetAddressOf(), _d3dContext.Get()));
}

DeviceResources::~DeviceResources()
{
	_renderBinding->UnloadAllTextures(*_render);
}

// This method is called in the event handler for the DisplayContentsInvalidated event.
// If return value is false a new D3D device must be created.
bool DeviceResources::ValidateDevice() const
{
	// The D3D Device is no longer valid if the default adapter changed since the device
	// was created or if the device has been removed.

	// First, get the information for the default adapter from when the device was created.

	ComPtr<IDXGIDevice3> dxgiDevice;
	DX::ThrowIfFailed(_d3dDevice.As(&dxgiDevice));

	ComPtr<IDXGIAdapter> deviceAdapter;
	DX::ThrowIfFailed(dxgiDevice->GetAdapter(&deviceAdapter));

	ComPtr<IDXGIFactory2> deviceFactory;
	DX::ThrowIfFailed(deviceAdapter->GetParent(IID_PPV_ARGS(&deviceFactory)));

	ComPtr<IDXGIAdapter1> previousDefaultAdapter;
	DX::ThrowIfFailed(deviceFactory->EnumAdapters1(0, &previousDefaultAdapter));

	DXGI_ADAPTER_DESC previousDesc;
	DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));

	// Next, get the information for the current default adapter.

	ComPtr<IDXGIFactory2> currentFactory;
	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentFactory)));

	ComPtr<IDXGIAdapter1> currentDefaultAdapter;
	DX::ThrowIfFailed(currentFactory->EnumAdapters1(0, &currentDefaultAdapter));

	DXGI_ADAPTER_DESC currentDesc;
	DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));

	// If the adapter LUIDs don't match, or if the device reports that it has been removed,
	// a new D3D device must be created.

	return previousDesc.AdapterLuid.LowPart == currentDesc.AdapterLuid.LowPart &&
		previousDesc.AdapterLuid.HighPart == currentDesc.AdapterLuid.HighPart &&
		!IsDeviceRemoved();
}

bool DeviceResources::IsDeviceRemoved() const
{
	return FAILED(_d3dDevice->GetDeviceRemovedReason());
}

IRender& DeviceResources::GetRender(int rotationAngle, int width, int height)
{
	if (m_rotationAngle != rotationAngle || m_pxWidth != width || m_pxHeight != height)
	{
		HRESULT hr = ResizeSwapChainInternal(width, height, rotationAngle);
		if (!DX::IsDeviceLost(hr))
			DX::ThrowIfFailed(hr);
	}

	return *_render;
}

void DeviceResources::Present()
{
	// The first argument instructs DXGI to block until VSync, putting the application
	// to sleep until the next VSync. This ensures we don't waste any cycles rendering
	// frames that will never be displayed to the screen.
	HRESULT hr = _swapChain->Present(1, 0);

	if (DX::IsDeviceLost(hr))
	{
		// Do nothing - the main loop will check for device lost and recover on next tick
	}
	else if (FAILED(hr))
	{
		throw Platform::Exception::CreateException(hr);
	}
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

HRESULT DeviceResources::ResizeSwapChainInternal(int width, int height, int rotationAngle)
{
	assert(_swapChain != nullptr);

	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	_d3dContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	_d3dRenderTargetView.Reset();
	_d3dContext->Flush();

	// Prevent zero size DirectX content from being created.
	int outputWidth = std::max(width, 1);
	int outputHeight = std::max(height, 1);

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	bool swapDimensions = 90 == rotationAngle || 270 == rotationAngle;
	int renderTargetWidth = swapDimensions ? outputHeight : outputWidth;
	int renderTargetHeight = swapDimensions ? outputWidth : outputHeight;

	ReturnIfFailed(_swapChain->ResizeBuffers(
		2, // Double-buffered swap chain.
		renderTargetWidth,
		renderTargetHeight,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		0));

	ReturnIfFailed(_swapChain->SetRotation(AsDXGIModeRotation(rotationAngle)));

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	ReturnIfFailed(_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

	ReturnIfFailed(_d3dDevice->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&_d3dRenderTargetView));

	m_rotationAngle = rotationAngle;
	m_pxWidth = width;
	m_pxHeight = height;

	return S_OK;
}

#include "pch.h"
#include "DeviceResources12.h"
#include "DirectXHelper.h"
#include <video/RenderD3D12.h>
#include <video/RenderBinding.h>

using namespace DirectX;
using namespace Microsoft::WRL;
using namespace Windows::Foundation;
using namespace Windows::UI::Core;
using namespace Platform;

#define ReturnIfFailed(expr) if(HRESULT hr = (expr); SUCCEEDED(hr)) {} else return hr

static ComPtr<IDXGISwapChain3> CreateSwapchainForCoreWindow(IDXGIFactory2* dxgiFactory, IUnknown* deviceForSwapChain, CoreWindow^ coreWindow)
{
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = { 0 };
	swapChainDesc.Width = 0; // todo: Match the size of the window.
	swapChainDesc.Height = 0;
	swapChainDesc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	swapChainDesc.Stereo = false;
	swapChainDesc.SampleDesc.Count = 1;
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

// Acquires the first available hardware adapter that supports Direct3D 12.
static ComPtr<IDXGIAdapter1> GetHardwareAdapter(IDXGIFactory1 *dxgiFactory)
{
	ComPtr<IDXGIAdapter1> adapter;

	for (UINT adapterIndex = 0; DXGI_ERROR_NOT_FOUND != dxgiFactory->EnumAdapters1(adapterIndex, &adapter); adapterIndex++)
	{
		DXGI_ADAPTER_DESC1 desc;
		adapter->GetDesc1(&desc);

		if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
		{
			// Don't select the Basic Render Driver adapter.
			continue;
		}

		// Check to see if the adapter supports Direct3D 12, but don't create the actual device yet.
		if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), nullptr)))
		{
			break;
		}
	}

	return adapter;
}

DeviceResources12::DeviceResources12(CoreWindow^ coreWindow)
	: _renderBinding(new RenderBinding())
	, _displayOrientation(DO_0)
{
#if defined(_DEBUG)
	// If the project is in a debug build, enable debugging via SDK Layers.
	{
		ComPtr<ID3D12Debug> debugController;
		if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
		{
			debugController->EnableDebugLayer();
		}
	}
#endif

	DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&_dxgiFactory)));

	ComPtr<IDXGIAdapter1> adapter = GetHardwareAdapter(_dxgiFactory.Get());

	// Create the Direct3D 12 API device object
	HRESULT hr = D3D12CreateDevice(
		adapter.Get(),                  // The hardware adapter.
		D3D_FEATURE_LEVEL_11_0,         // Minimum feature level this app can support.
		IID_PPV_ARGS(&_d3dDevice)       // Returns the Direct3D device created.
	);
	if (FAILED(hr))
	{
		// If the initialization fails, fall back to the WARP device.
		// For more information on WARP, see: 
		// https://go.microsoft.com/fwlink/?LinkId=286690

		ComPtr<IDXGIAdapter> warpAdapter;
		DX::ThrowIfFailed(_dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

		hr = D3D12CreateDevice(warpAdapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&_d3dDevice));
	}
	DX::ThrowIfFailed(hr);

	// Create the command queue.
	D3D12_COMMAND_QUEUE_DESC queueDesc = {};
	queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	DX::ThrowIfFailed(_d3dDevice->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&_commandQueue)));

	// Create descriptor heaps for render target views and depth stencil views.
	D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
	rtvHeapDesc.NumDescriptors = c_frameCount;
	rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DX::ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&_rtvHeap)));

	_rtvDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	DX::ThrowIfFailed(_d3dDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(&_dsvHeap)));

	for (UINT n = 0; n < c_frameCount; n++)
	{
		DX::ThrowIfFailed(
			_d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocators[n]))
		);
	}

	// Create synchronization objects.
	DX::ThrowIfFailed(_d3dDevice->CreateFence(_fenceValues[_currentFrame], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_fence)));
	_fenceValues[_currentFrame]++;

	_fenceEvent.reset(CreateEvent(nullptr, FALSE, FALSE, nullptr));
	if (!_fenceEvent)
	{
		DX::ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
	}

	// Swap chains need a reference to the command queue in DirectX 12
	_swapChain = CreateSwapchainForCoreWindow(_dxgiFactory.Get(), _commandQueue.Get(), coreWindow);

	_render.reset(new RenderD3D12(_d3dDevice.Get(), _commandQueue.Get()));
}

DeviceResources12::~DeviceResources12()
{
	_renderBinding->UnloadAllTextures(*_render);
}

// This method is called in the event handler for the DisplayContentsInvalidated event.
// If return value is false a new D3D device must be created.
bool DeviceResources12::ValidateDevice() const
{
	// The D3D Device is no longer valid if the default adapter changed since the device
	// was created or if the device has been removed.

	// First, get the LUID for the default adapter from when the device was created.
	DXGI_ADAPTER_DESC previousDesc;
	{
		ComPtr<IDXGIAdapter1> previousDefaultAdapter;
		DX::ThrowIfFailed(_dxgiFactory->EnumAdapters1(0, &previousDefaultAdapter));

		DX::ThrowIfFailed(previousDefaultAdapter->GetDesc(&previousDesc));
	}

	// Next, get the information for the current default adapter.
	DXGI_ADAPTER_DESC currentDesc;
	{
		ComPtr<IDXGIFactory4> currentDxgiFactory;
		DX::ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(&currentDxgiFactory)));

		ComPtr<IDXGIAdapter1> currentDefaultAdapter;
		DX::ThrowIfFailed(currentDxgiFactory->EnumAdapters1(0, &currentDefaultAdapter));

		DX::ThrowIfFailed(currentDefaultAdapter->GetDesc(&currentDesc));
	}

	// If the adapter LUIDs don't match, or if the device reports that it has been removed,
	// a new D3D device must be created.
	return previousDesc.AdapterLuid.LowPart == currentDesc.AdapterLuid.LowPart &&
		previousDesc.AdapterLuid.HighPart == currentDesc.AdapterLuid.HighPart &&
		SUCCEEDED(_d3dDevice->GetDeviceRemovedReason());
}

bool DeviceResources12::IsDeviceRemoved() const
{
	return FAILED(_d3dDevice->GetDeviceRemovedReason());
}

IRender& DeviceResources12::GetRender(int width, int height, DisplayOrientation displayOrientation)
{
	if (_displayOrientation != displayOrientation || _pxWidth != width || _pxHeight != height)
	{
		HRESULT hr = ResizeSwapChainInternal(width, height, displayOrientation);
		if (!DX::IsDeviceLost(hr))
			DX::ThrowIfFailed(hr);
	}

	_render->Begin(width, height, displayOrientation);

	return *_render;
}

void DeviceResources12::Present()
{
	_render->End();

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

static DXGI_MODE_ROTATION AsDXGIModeRotation(DisplayOrientation displayOrientation)
{
	switch (displayOrientation)
	{
	default: assert(0);
	case DO_0: return DXGI_MODE_ROTATION_IDENTITY;
	case DO_90: return DXGI_MODE_ROTATION_ROTATE90;
	case DO_180: return DXGI_MODE_ROTATION_ROTATE180;
	case DO_270: return DXGI_MODE_ROTATION_ROTATE270;
	}
}

HRESULT DeviceResources12::ResizeSwapChainInternal(int width, int height, DisplayOrientation displayOrientation)
{
	assert(_swapChain != nullptr);

	// Wait until all previous GPU work is complete.
//	WaitForGpu();

	// Clear the previous window size specific content and update the tracked fence values.
	for (UINT n = 0; n < c_frameCount; n++)
	{
		_renderTargets[n].Reset();
		_fenceValues[n] = _fenceValues[_currentFrame];
	}

	// Prevent zero size DirectX content from being created.
	int outputWidth = std::max(width, 1);
	int outputHeight = std::max(height, 1);

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	bool swapDimensions = DO_90 == displayOrientation || DO_270 == displayOrientation;
	int renderTargetWidth = swapDimensions ? outputHeight : outputWidth;
	int renderTargetHeight = swapDimensions ? outputWidth : outputHeight;

	ReturnIfFailed(_swapChain->ResizeBuffers(
		2, // Double-buffered swap chain.
		renderTargetWidth,
		renderTargetHeight,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		0));

	ReturnIfFailed(_swapChain->SetRotation(AsDXGIModeRotation(displayOrientation)));

	// Create render target views of the swap chain back buffer.
	_currentFrame = _swapChain->GetCurrentBackBufferIndex();
	D3D12_CPU_DESCRIPTOR_HANDLE rtvDescriptor = _rtvHeap->GetCPUDescriptorHandleForHeapStart();
	for (UINT n = 0; n < c_frameCount; n++)
	{
		DX::ThrowIfFailed(_swapChain->GetBuffer(n, IID_PPV_ARGS(&_renderTargets[n])));
		_d3dDevice->CreateRenderTargetView(_renderTargets[n].Get(), nullptr, rtvDescriptor);
		rtvDescriptor.ptr += _rtvDescriptorSize;
	}

	_displayOrientation = displayOrientation;
	_pxWidth = width;
	_pxHeight = height;

	return S_OK;
}

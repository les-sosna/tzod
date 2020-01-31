#define NOMINMAX
#define WIN32_LEAN_AND_MEAN
#include "inc/video/SwapChainResources.h"
#include <dxgi1_4.h>
#include <d3d11_2.h>
#include <wrl/client.h>
#include <cassert>
#include <algorithm>

#define ReturnIfFailed(expr) if(HRESULT hr = (expr); SUCCEEDED(hr)) {} else return hr

using namespace Microsoft::WRL;

SwapChainResources::SwapChainResources(IDXGISwapChain3 *swapChain)
	: m_swapChain(swapChain)
{
}

SwapChainResources::~SwapChainResources()
{
}

HRESULT SwapChainResources::SetPixelSize(ID3D11Device *device, ID3D11DeviceContext2 *deviceContext, int width, int height)
{
	if (m_pxWidth != width || m_pxHeight != height)
	{
		m_pxWidth = width;
		m_pxHeight = height;
		if (m_rotationAngle != -1)
		{
			ReturnIfFailed(ResizeSwapChainInternal(device, deviceContext, m_pxWidth, m_pxHeight, m_rotationAngle));
		}
	}
	return S_OK;
}

// This method is called in the event handler for the OrientationChanged event.
HRESULT SwapChainResources::SetCurrentOrientation(ID3D11Device *device, ID3D11DeviceContext2 *deviceContext, int rotationAngle)
{
	if (m_rotationAngle != rotationAngle)
	{
		m_rotationAngle = rotationAngle;
		if (m_pxWidth != 0 || m_pxHeight != 0)
		{
			ReturnIfFailed(ResizeSwapChainInternal(device, deviceContext, m_pxWidth, m_pxHeight, m_rotationAngle));
		}
	}
	return S_OK;
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

HRESULT SwapChainResources::ResizeSwapChainInternal(ID3D11Device *device, ID3D11DeviceContext2 *deviceContext, int width, int height, int rotationAngle)
{
	assert(m_swapChain != nullptr);

	// Clear the previous window size specific context.
	ID3D11RenderTargetView* nullViews[] = { nullptr };
	deviceContext->OMSetRenderTargets(ARRAYSIZE(nullViews), nullViews, nullptr);
	m_d3dRenderTargetView.Reset();
	deviceContext->Flush();

	// Prevent zero size DirectX content from being created.
	int outputWidth = std::max(width, 1);
	int outputHeight = std::max(height, 1);

	// The width and height of the swap chain must be based on the window's
	// natively-oriented width and height. If the window is not in the native
	// orientation, the dimensions must be reversed.
	bool swapDimensions = 90 == rotationAngle || 270 == rotationAngle;
	int renderTargetWidth = swapDimensions ? outputHeight : outputWidth;
	int renderTargetHeight = swapDimensions ? outputWidth : outputHeight;

	ReturnIfFailed(m_swapChain->ResizeBuffers(
		2, // Double-buffered swap chain.
		renderTargetWidth,
		renderTargetHeight,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		0));

	ReturnIfFailed(m_swapChain->SetRotation(AsDXGIModeRotation(rotationAngle)));

	// Create a render target view of the swap chain back buffer.
	ComPtr<ID3D11Texture2D> backBuffer;
	ReturnIfFailed(m_swapChain->GetBuffer(0, IID_PPV_ARGS(&backBuffer)));

	ReturnIfFailed(device->CreateRenderTargetView(
		backBuffer.Get(),
		nullptr,
		&m_d3dRenderTargetView));

	return true;
}

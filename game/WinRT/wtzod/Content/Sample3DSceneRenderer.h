#pragma once

#include "ShaderStructures.h"
#include "StepTimer.h"

namespace DX
{
	class DeviceResources;
	class SwapChainResources;
}

namespace wtzod
{
	// This sample renderer instantiates a basic rendering pipeline.
	class Sample3DSceneRenderer
	{
	public:
		Sample3DSceneRenderer();
		~Sample3DSceneRenderer();

		void SetDeviceResources(DX::DeviceResources *deviceResources);
		void SetSwapChainResources(DX::SwapChainResources *swapChainResources);

		void Update(DX::StepTimer const& timer);
		void Render();

	private:
		void Rotate(float radians);

	private:
		DX::DeviceResources* m_deviceResources;
		DX::SwapChainResources* m_swapChainResources;

		// Direct3D resources for cube geometry.
		Microsoft::WRL::ComPtr<ID3D11InputLayout>	m_inputLayout;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_vertexBuffer;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_indexBuffer;
		Microsoft::WRL::ComPtr<ID3D11VertexShader>	m_vertexShader;
		Microsoft::WRL::ComPtr<ID3D11PixelShader>	m_pixelShader;
		Microsoft::WRL::ComPtr<ID3D11Buffer>		m_constantBuffer;

		// System resources for cube geometry.
		ModelViewProjectionConstantBuffer	m_constantBufferData;
		uint32	m_indexCount;

		// Variables used with the rendering loop.
		float	m_degreesPerSecond;
		bool	m_loadingComplete;
	};
}


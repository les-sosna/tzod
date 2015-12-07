#pragma once
#include <wrl/client.h>

enum D3D_FEATURE_LEVEL;
struct ID3D11Device2;
struct ID3D11DeviceContext2;

namespace DX
{
	// Controls all the DirectX device resources.
	class DeviceResources
	{
	public:
		DeviceResources();
		bool ValidateDevice() const;
		void Trim();

		// D3D Accessors.
		ID3D11Device2*			GetD3DDevice() const					{ return m_d3dDevice.Get(); }
		ID3D11DeviceContext2*	GetD3DDeviceContext() const				{ return m_d3dContext.Get(); }
		D3D_FEATURE_LEVEL		GetDeviceFeatureLevel() const			{ return m_d3dFeatureLevel; }

	private:
		// Direct3D objects.
		Microsoft::WRL::ComPtr<ID3D11Device2>			m_d3dDevice;
		Microsoft::WRL::ComPtr<ID3D11DeviceContext2>	m_d3dContext;

		// Cached device properties.
		D3D_FEATURE_LEVEL								m_d3dFeatureLevel;
	};
}
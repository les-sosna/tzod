#pragma once
#include "RenderBase.h"
#include <math/MyMath.h>
#include <wrl/client.h>
#include <memory>
#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048

struct ID3D11DeviceContext2;
struct ID3D11RasterizerState;
struct ID3D11BlendState;
struct ID3D11SamplerState;
struct ID3D11VertexShader;
struct ID3D11PixelShader;
struct ID3D11InputLayout;
struct ID3D11Buffer;
struct ID3D11Device;
struct ID3D11RenderTargetView;

class SwapChainResources;

class RenderD3D11 : public IRender
{
public:
	RenderD3D11(ID3D11DeviceContext2 *context, SwapChainResources &swapChainResources);
	~RenderD3D11() override;

	// IRender
	void OnResizeWnd(unsigned int width, unsigned int height) override;
	void SetDisplayOrientation(DisplayOrientation displayOrientation) override;

	void SetViewport(const RectRB *rect) override;
	void SetScissor(const RectRB *rect) override;
	void Camera(const RectRB *vp, float x, float y, float scale) override;

	void Begin() override;
	void End() override;
	void SetMode(const RenderMode mode) override;

	void SetAmbient(float ambient) override;

	bool TexCreate(DEV_TEXTURE &tex, const Image &img, bool magFilter) override;
	void TexFree(DEV_TEXTURE tex) override;

	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;

	void DrawLines(const MyLine *lines, size_t count) override;

private:
	void Flush();

	SwapChainResources &_swapChainResources;
	Microsoft::WRL::ComPtr<ID3D11Device>        _device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext2> _context;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        _constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        _vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        _indexBuffer;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   _inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  _vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   _pixelShaderColor;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   _pixelShaderLight;
	Microsoft::WRL::ComPtr<ID3D11SamplerState>  _samplerState;
	Microsoft::WRL::ComPtr<ID3D11BlendState>    _blendStateUI;
	Microsoft::WRL::ComPtr<ID3D11BlendState>    _blendStateWorld;
	Microsoft::WRL::ComPtr<ID3D11BlendState>    _blendStateLight;
	Microsoft::WRL::ComPtr<ID3D11RasterizerState> _rasterizerState;

	int _windowWidth;
	int _windowHeight;
	RectRB _viewport;
	vec2d _cameraEye;
	float _cameraScale;
	DisplayOrientation _displayOrientation;

	void* _curtex;
	float _ambient;

	UINT16 _indexArray[INDEX_ARRAY_SIZE];
	MyVertex _vertexArray[VERTEX_ARRAY_SIZE];

	unsigned int _vaSize;      // number of filled elements in _vertexArray
	unsigned int _iaSize;      // number of filled elements in _indexArray

	RenderMode  _mode;
};

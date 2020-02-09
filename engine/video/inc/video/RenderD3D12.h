#pragma once
#include "RenderBase.h"
#include <math/MyMath.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <memory>
#define VERTEX_ARRAY_SIZE   1024
#define INDEX_ARRAY_SIZE    2048

class RenderD3D12 : public IRender
{
public:
	RenderD3D12(ID3D12Device* d3dDevice);
	~RenderD3D12() override;

	// IRender
	void SetViewport(const RectRB &rect) override;
	void SetScissor(const RectRB &rect) override;
	void SetTransform(vec2d offset, float scale) override;

	void Begin(unsigned int displayWidth, unsigned int displayHeight, DisplayOrientation displayOrientation) override;
	void End() override;
	void SetMode(const RenderMode mode) override;

	void SetAmbient(float ambient) override;

	bool TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter) override;
	void TexFree(DEV_TEXTURE tex) override;

	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;

	void DrawLines(const MyLine *lines, size_t count) override;

private:
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineStateUI;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineStateWorld;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineStateLight;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _constantBuffer;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;

	unsigned int _windowWidth = 0;
	unsigned int _windowHeight = 0;
	RectRB _viewport = {};
	vec2d _offset = {};
	float _scale = 1;
	DisplayOrientation _displayOrientation = DO_0;

	void* _curtex = nullptr;
	float _ambient = 1.f;

	UINT16 _indexArray[INDEX_ARRAY_SIZE];
	MyVertex _vertexArray[VERTEX_ARRAY_SIZE];

	unsigned int _vaSize = 0; // number of filled elements in _vertexArray
	unsigned int _iaSize = 0; // number of filled elements in _indexArray

	RenderMode  _mode;
};

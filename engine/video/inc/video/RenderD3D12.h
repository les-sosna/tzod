#pragma once
#include "RenderBase.h"
#include <math/MyMath.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <memory>
#include <list>
#define VERTEX_ARRAY_SIZE   1024
#define INDEX_ARRAY_SIZE    2048

class RenderD3D12 : public IRender
{
public:
	RenderD3D12(ID3D12Device *d3dDevice, ID3D12CommandQueue *commandQueue);
	~RenderD3D12() override;

	void Begin(ID3D12Resource *rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv, int width, int height, DisplayOrientation displayOrientation);
	void End(ID3D12Resource* rt);

	// IRender
	void SetViewport(const RectRB &rect) override;
	void SetScissor(const RectRB &rect) override;
	void SetTransform(vec2d offset, float scale) override;
	void SetMode(const RenderMode mode) override;
	void SetAmbient(float ambient) override;
	bool TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter) override;
	void TexFree(DEV_TEXTURE tex) override;
	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;
	void DrawLines(const MyLine *lines, size_t count) override;

private:
	void Flush();

	Microsoft::WRL::ComPtr<ID3D12Device> _d3dDevice;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> _commandQueue;
	Microsoft::WRL::ComPtr<ID3D12RootSignature> _rootSignature;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineStateUI;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineStateWorld;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> _pipelineStateLight;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> _commandAllocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> _commandList;
	Microsoft::WRL::ComPtr<ID3D12Resource> _vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _indexBuffer;
	Microsoft::WRL::ComPtr<ID3D12Resource> _constantBuffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> _samplerHeap;
	D3D12_VERTEX_BUFFER_VIEW _vertexBufferView;
	D3D12_INDEX_BUFFER_VIEW _indexBufferView;
	D3D12_CPU_DESCRIPTOR_HANDLE _rtv = {};

	struct TextureData
	{
		Microsoft::WRL::ComPtr<ID3D12Resource> resource;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srvHeap;
		D3D12_GPU_DESCRIPTOR_HANDLE sampler;
	};
	std::list<TextureData> _textures;

	int _windowWidth = 0;
	int _windowHeight = 0;
	RectRB _scissor = {};
	RectRB _viewport = {};
	vec2d _offset = {};
	float _scale = 1;
	DisplayOrientation _displayOrientation = DO_0;

	TextureData *_curtex = nullptr;
	float _ambient = 1.f;

	UINT16 _indexArray[INDEX_ARRAY_SIZE] = {};
	MyVertex _vertexArray[VERTEX_ARRAY_SIZE] = {};

	unsigned int _vaSize = 0; // number of filled elements in _vertexArray
	unsigned int _iaSize = 0; // number of filled elements in _indexArray

	RenderMode  _mode;
};

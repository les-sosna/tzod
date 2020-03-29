#include "inc/video/RenderD3D12.h"
#include <d3d12.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <cstring>
#include <stdexcept>

using namespace Microsoft::WRL;

struct MyConstants
{
	DirectX::XMFLOAT4X4 viewProj;
};

static const char s_vertexShader[] =
R"(
	cbuffer cb : register(b0)
	{
		float4x4 viewProj;
	};

	struct VertexShaderInput
	{
		float3 pos : POSITION;
		float4 color : COLOR;
		float2 texcoord : TEXCOORD;
	};

	struct PixelShaderInput
	{
		float4 hpos : SV_Position;
		float4 color : COLOR;
		float2 texcoord : TEXCOORD;
	};

	PixelShaderInput main(VertexShaderInput input)
	{
		PixelShaderInput output;

		output.hpos = mul(float4(input.pos.xy, 0, 1), viewProj);
		output.color = input.color;
		output.texcoord = input.texcoord;

		return output;
	}
)";

static const char s_pixelShaderColor[] =
R"(
	struct PixelShaderInput
	{
		float4 hpos : SV_Position;
		float4 color : COLOR;
		float2 texcoord : TEXCOORD;
	};

	Texture2D tex : register(t0);
	SamplerState sam : register(s0);

	float4 main(PixelShaderInput input) : SV_Target
	{
		return input.color * tex.Sample(sam, input.texcoord);
	}
)";

static const char s_pixelShaderLight[] =
R"(
	struct PixelShaderInput
	{
		float4 hpos : SV_Position;
		float4 color : COLOR;
		float2 texcoord : TEXCOORD;
	};

	float4 main(PixelShaderInput input) : SV_Target
	{
		return input.color;
	}
)";

static void ThrowIfFailed(HRESULT hr, const char *msg)
{
	if (FAILED(hr))
	{
		throw std::runtime_error(msg);
	}
}

#define TOSTR(s) #s
#define CHECK(expr) ThrowIfFailed((expr), "Failed at line " TOSTR(__LINE__))

// Assign a name to the object to aid with debugging.
#if NDEBUG
inline void SetD3D12ObjectName(ID3D12Object*, LPCWSTR) {}
#else
inline void SetD3D12ObjectName(ID3D12Object* o, LPCWSTR name) { o->SetName(name); }
#endif
#define NAME_D3D12_OBJECT(x) SetD3D12ObjectName((x).Get(), L#x)


///////////////////////////////////////////////////////////////////////////////

static const DirectX::XMFLOAT4X4 s_rotation0(
	 1.f, 0.f, 0.f, 0.f,
	 0.f, 1.f, 0.f, 0.f,
	 0.f, 0.f, 1.f, 0.f,
	 0.f, 0.f, 0.f, 1.f);

static const DirectX::XMFLOAT4X4 s_rotation90(
	 0.f, 1.f, 0.f, 0.f,
	-1.f, 0.f, 0.f, 0.f,
	 0.f, 0.f, 1.f, 0.f,
	 0.f, 0.f, 0.f, 1.f);

static const DirectX::XMFLOAT4X4 s_rotation180(
	-1.f, 0.f, 0.f, 0.f,
	 0.f,-1.f, 0.f, 0.f,
	 0.f, 0.f, 1.f, 0.f,
	 0.f, 0.f, 0.f, 1.f);

static const DirectX::XMFLOAT4X4 s_rotation270(
	 0.f,-1.f, 0.f, 0.f,
	 1.f, 0.f, 0.f, 0.f,
	 0.f, 0.f, 1.f, 0.f,
	 0.f, 0.f, 0.f, 1.f);

static RectRB ApplyRectRotation(const RectRB &rect, int windowWidth, int windowHeight, DisplayOrientation orientation)
{
	RectRB result;
	switch (orientation)
	{
	default:
		assert(false);
	case DO_0:
		result = rect;
		break;
	case DO_90:
		result.left = windowHeight - HEIGHT(rect) - rect.top;
		result.top = rect.left;
		result.right = windowHeight - rect.top;
		result.bottom = rect.left + WIDTH(rect);
		break;
	case DO_180:
		result.left = windowWidth - WIDTH(rect) - rect.left;
		result.top = windowHeight - HEIGHT(rect) - rect.top;
		result.right = windowWidth - rect.left;
		result.bottom = windowHeight - rect.top;
		break;
	case DO_270:
		result.left = rect.top;
		result.top = windowWidth - WIDTH(rect) - rect.left;
		result.right = rect.top + HEIGHT(rect);
		result.bottom = windowWidth - rect.left;
		break;
	}
	return result;
}

static D3D12_RECT AsD3D12Rect(const RectRB &rect)
{
	return D3D12_RECT{ rect.left, rect.top, rect.right, rect.bottom };
}

static D3D12_VIEWPORT AsD3D12Viewport(const RectRB &rect)
{
	return D3D12_VIEWPORT
	{
		(FLOAT)rect.left,
		(FLOAT)rect.top,
		(FLOAT)WIDTH(rect),
		(FLOAT)HEIGHT(rect),
		0.f, // MinDepth
		1.f // MaxDepth
	};
}

static const DirectX::XMFLOAT4X4* GetOrientationTransform(DisplayOrientation orientation)
{
	switch (orientation)
	{
	default:
		assert(false);
	case DO_0:
		return &s_rotation0;
	case DO_90:
		return &s_rotation90;
	case DO_180:
		return &s_rotation180;
	case DO_270:
		return &s_rotation270;
	}
}

static float GetProjWidth(const RectRB &viewport, DisplayOrientation orientation)
{
	return (float)((DO_90 == orientation || DO_270 == orientation) ? HEIGHT(viewport) : WIDTH(viewport));
}

static float GetProjHeight(const RectRB &viewport, DisplayOrientation orientation)
{
	return (float)((DO_90 == orientation || DO_270 == orientation) ? WIDTH(viewport) : HEIGHT(viewport));
}

///////////////////////////////////////////////////////////////////////////////

enum// RootParameterIndex
{
	RootParameterCBV,
	RootParameterSRV,
	RootParameterSampler,
	_RootParameterCount,
};

static ComPtr<ID3D12RootSignature> MakeRootSignature(ID3D12Device* d3dDevice)
{
	D3D12_DESCRIPTOR_RANGE srvPS[1] = {};
	srvPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
	srvPS[0].NumDescriptors = 1;
	srvPS[0].BaseShaderRegister = 0;
	srvPS[0].RegisterSpace = 0;
	srvPS[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_DESCRIPTOR_RANGE samplerPS[1] = {};
	samplerPS[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SAMPLER;
	samplerPS[0].NumDescriptors = 1;
	samplerPS[0].BaseShaderRegister = 0;
	samplerPS[0].RegisterSpace = 0;
	samplerPS[0].OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

	D3D12_ROOT_PARAMETER parameters[_RootParameterCount] = {};

	parameters[RootParameterCBV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
	parameters[RootParameterCBV].Descriptor.ShaderRegister = 0;
	parameters[RootParameterCBV].Descriptor.RegisterSpace = 0;
	parameters[RootParameterCBV].ShaderVisibility = D3D12_SHADER_VISIBILITY_VERTEX;

	parameters[RootParameterSRV].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameters[RootParameterSRV].DescriptorTable.NumDescriptorRanges = _countof(srvPS);
	parameters[RootParameterSRV].DescriptorTable.pDescriptorRanges = srvPS;
	parameters[RootParameterSRV].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	parameters[RootParameterSampler].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
	parameters[RootParameterSampler].DescriptorTable.NumDescriptorRanges = _countof(samplerPS);
	parameters[RootParameterSampler].DescriptorTable.pDescriptorRanges = samplerPS;
	parameters[RootParameterSampler].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

	// Only the input assembler stage needs access to the constant buffer
	D3D12_ROOT_SIGNATURE_DESC descRootSignature = {};
	descRootSignature.NumParameters = _countof(parameters);
	descRootSignature.pParameters = parameters;
	descRootSignature.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS |
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS;

	ComPtr<ID3DBlob> signature;
	ComPtr<ID3DBlob> error;
	CHECK(D3D12SerializeRootSignature(&descRootSignature, D3D_ROOT_SIGNATURE_VERSION_1, signature.GetAddressOf(), error.GetAddressOf()));

	ComPtr<ID3D12RootSignature> rootSignature;
	CHECK(d3dDevice->CreateRootSignature(0, signature->GetBufferPointer(), signature->GetBufferSize(), IID_PPV_ARGS(&rootSignature)));

	return rootSignature;
}

template <class T>
static T OffsetDescriptorHandle(T handle, INT offset)
{
	return T{ handle.ptr + offset };
}

static constexpr D3D12_RENDER_TARGET_BLEND_DESC c_defaultRenderTargetBlendDesc = {
	FALSE,                      // BlendEnable
	FALSE,                      // LogicOpEnable
	D3D12_BLEND_ONE,            // SrcBlend
	D3D12_BLEND_ZERO,           // DestBlend
	D3D12_BLEND_OP_ADD,         // BlendOp
	D3D12_BLEND_ONE,            // SrcBlendAlpha
	D3D12_BLEND_ZERO,           // DestBlendAlpha
	D3D12_BLEND_OP_ADD,         // BlendOpAlpha
	D3D12_LOGIC_OP_NOOP,        // LogicOp
	D3D12_COLOR_WRITE_ENABLE_ALL, // RenderTargetWriteMask
};

static constexpr D3D12_RASTERIZER_DESC c_defaultRasterizerDesc = {
	D3D12_FILL_MODE_SOLID,      // FillMode
	D3D12_CULL_MODE_BACK,       // CullMode;
	FALSE,                      // FrontCounterClockwise
	D3D12_DEFAULT_DEPTH_BIAS,   // DepthBias
	D3D12_DEFAULT_DEPTH_BIAS_CLAMP, // DepthBiasClamp
	D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS, // SlopeScaledDepthBias
	TRUE,                       // DepthClipEnable
	FALSE,                      // MultisampleEnable
	FALSE,                      // AntialiasedLineEnable
	0,                          // ForcedSampleCount
	D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF // ConservativeRaster
};

static constexpr D3D12_HEAP_PROPERTIES c_uploadHeapProps = {
	D3D12_HEAP_TYPE_UPLOAD,     // Type
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // CPUPageProperty
	D3D12_MEMORY_POOL_UNKNOWN,  // MemoryPoolPreference
	1,                          // CreationNodeMask
	1                           // VisibleNodeMask
};

static constexpr D3D12_HEAP_PROPERTIES c_defaultHeapProps = {
	D3D12_HEAP_TYPE_DEFAULT,    // Type
	D3D12_CPU_PAGE_PROPERTY_UNKNOWN, // CPUPageProperty
	D3D12_MEMORY_POOL_UNKNOWN,  // MemoryPoolPreference
	1,                          // CreationNodeMask
	1                           // VisibleNodeMask
};


RenderD3D12::RenderD3D12(ID3D12Device* d3dDevice, ID3D12CommandQueue* commandQueue)
	: _d3dDevice(d3dDevice)
	, _commandQueue(commandQueue)
{
	D3D12_DESCRIPTOR_HEAP_DESC samplerHeapDesc = {};
	samplerHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER;
	samplerHeapDesc.NumDescriptors = 2; // linear and point
	samplerHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHECK(_d3dDevice->CreateDescriptorHeap(&samplerHeapDesc, IID_PPV_ARGS(&_samplerHeap)));
	NAME_D3D12_OBJECT(_samplerHeap);

	auto sampleDescriptorSize = _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER);

	D3D12_SAMPLER_DESC samplerDesc = {};
	samplerDesc.AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
	samplerDesc.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
	samplerDesc.MipLODBias = 0;
	samplerDesc.MaxAnisotropy = 0;
	samplerDesc.ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
	samplerDesc.MinLOD = 0.0f;
	samplerDesc.MaxLOD = D3D12_FLOAT32_MAX;

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_LINEAR;
	_d3dDevice->CreateSampler(&samplerDesc, _samplerHeap->GetCPUDescriptorHandleForHeapStart());

	samplerDesc.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
	_d3dDevice->CreateSampler(&samplerDesc, OffsetDescriptorHandle(_samplerHeap->GetCPUDescriptorHandleForHeapStart(), sampleDescriptorSize));

	_rootSignature = MakeRootSignature(d3dDevice);
	NAME_D3D12_OBJECT(_rootSignature);


	ComPtr<ID3DBlob> log;

	// Shaders
	ComPtr<ID3DBlob> codeVS;
	if (FAILED(D3DCompile(
		s_vertexShader,                 // pSrcData
		sizeof(s_vertexShader),         // SrcDataSize
		"vs.hlsl",                      // pSourceName
		nullptr,                        // [opt] pDefines
		nullptr,                        // [opt] pInclude
		"main",                         // [opt] pEntrypoint
		"vs_4_0_level_9_1",             // pTarget
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
		0,                              // Flags2
		codeVS.ReleaseAndGetAddressOf(), // [out] ppCode
		log.ReleaseAndGetAddressOf()    // [out] ppErrorMsgs
		)))
	{
		const char *msg = log ? (const char*)log->GetBufferPointer() : "Shader compilation failed";
		throw std::runtime_error(msg);
	}

	ComPtr<ID3DBlob> codeColorPS;
	if (FAILED(D3DCompile(
		s_pixelShaderColor,             // pSrcData
		sizeof(s_pixelShaderColor),     // SrcDataSize
		"ps",                           // pSourceName
		nullptr,                        // [opt] pDefines
		nullptr,                        // [opt] pInclude
		"main",                         // [opt] pEntrypoint
		"ps_4_0_level_9_1",             // pTarget
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
		0,                              // Flags2
		codeColorPS.ReleaseAndGetAddressOf(), // [out] ppCode
		log.ReleaseAndGetAddressOf()    // [out] ppErrorMsgs
		)))
	{
		const char *msg = log ? (const char*)log->GetBufferPointer() : "Shader compilation failed";
		throw std::runtime_error(msg);
	}

	ComPtr<ID3DBlob> codeLightPS;
	if (FAILED(D3DCompile(
		s_pixelShaderLight,             // pSrcData
		sizeof(s_pixelShaderLight),     // SrcDataSize
		"ps",                           // pSourceName
		nullptr,                        // [opt] pDefines
		nullptr,                        // [opt] pInclude
		"main",                         // [opt] pEntrypoint
		"ps_4_0_level_9_1",             // pTarget
		D3DCOMPILE_ENABLE_STRICTNESS | D3DCOMPILE_OPTIMIZATION_LEVEL3,
		0,                              // Flags2
		codeLightPS.ReleaseAndGetAddressOf(), // [out] ppCode
		log.ReleaseAndGetAddressOf()    // [out] ppErrorMsgs
		)))
	{
		const char *msg = log ? (const char*)log->GetBufferPointer() : "Shader compilation failed";
		throw std::runtime_error(msg);
	}

	// InputLayout
	D3D12_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
	//SemanticName SemanticIndex Format InputSlot AlignedByteOffset InputSlotClass InstanceDataStepRate
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 16, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
	};

	// common pipeline state
	D3D12_GRAPHICS_PIPELINE_STATE_DESC state = {};
	state.pRootSignature = _rootSignature.Get();
	state.VS = D3D12_SHADER_BYTECODE{ codeVS->GetBufferPointer(), codeVS->GetBufferSize() };
	state.SampleMask = UINT_MAX;
	state.RasterizerState = c_defaultRasterizerDesc;
	state.InputLayout = { inputElementDescs, _countof(inputElementDescs) };
	state.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	state.NumRenderTargets = 1;
	state.RTVFormats[0] = DXGI_FORMAT_B8G8R8A8_UNORM;
	state.SampleDesc.Count = 1;

	// pipeline state - Light
	std::fill(std::begin(state.BlendState.RenderTarget), std::end(state.BlendState.RenderTarget), c_defaultRenderTargetBlendDesc);
	state.BlendState.RenderTarget[0].BlendEnable = TRUE;
	state.BlendState.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_SRC_ALPHA;
	state.BlendState.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ONE;
	state.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALPHA;
	state.PS = D3D12_SHADER_BYTECODE{ codeLightPS->GetBufferPointer(), codeLightPS->GetBufferSize() };
	CHECK(d3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&_pipelineStateLight)));
	NAME_D3D12_OBJECT(_pipelineStateLight);

	// pipeline state - UI
	std::fill(std::begin(state.BlendState.RenderTarget), std::end(state.BlendState.RenderTarget), c_defaultRenderTargetBlendDesc);
	state.BlendState.RenderTarget[0].BlendEnable = TRUE;
	state.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
	state.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	state.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;
	state.PS = D3D12_SHADER_BYTECODE{ codeColorPS->GetBufferPointer(), codeColorPS->GetBufferSize() };
	CHECK(d3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&_pipelineStateUI)));
	NAME_D3D12_OBJECT(_pipelineStateUI);

	// pipeline state - World
	std::fill(std::begin(state.BlendState.RenderTarget), std::end(state.BlendState.RenderTarget), c_defaultRenderTargetBlendDesc);
	state.BlendState.RenderTarget[0].BlendEnable = TRUE;
	state.BlendState.RenderTarget[0].SrcBlend = D3D12_BLEND_DEST_ALPHA;
	state.BlendState.RenderTarget[0].DestBlend = D3D12_BLEND_INV_SRC_ALPHA;
	state.BlendState.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_RED | D3D12_COLOR_WRITE_ENABLE_GREEN | D3D12_COLOR_WRITE_ENABLE_BLUE;
	state.PS = D3D12_SHADER_BYTECODE{ codeColorPS->GetBufferPointer(), codeColorPS->GetBufferSize() };
	CHECK(d3dDevice->CreateGraphicsPipelineState(&state, IID_PPV_ARGS(&_pipelineStateWorld)));
	NAME_D3D12_OBJECT(_pipelineStateWorld);

	// create ring buffer to suballocate constant, vertex and index data
	D3D12_RESOURCE_DESC bufferDesc = {};
	bufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	bufferDesc.Width = _ringAllocator.GetCapacity();
	bufferDesc.Height = 1;
	bufferDesc.DepthOrArraySize = 1;
	bufferDesc.MipLevels = 1;
	bufferDesc.SampleDesc.Count = 1;
	bufferDesc.SampleDesc.Quality = 0;
	bufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	CHECK(d3dDevice->CreateCommittedResource(
		&c_uploadHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&bufferDesc,
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr, // clear value
		IID_PPV_ARGS(&_ringBuffer)));
	NAME_D3D12_OBJECT(_ringBuffer);

	D3D12_RANGE emptyReadRange = {};
	CHECK(_ringBuffer->Map(0, &emptyReadRange, (void**)&_mappedRingBuffer));

	// command allocator and command list
	CHECK(d3dDevice->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(&_commandAllocator)));
	NAME_D3D12_OBJECT(_commandAllocator);

	CHECK(d3dDevice->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, _commandAllocator.Get(), nullptr, IID_PPV_ARGS(&_commandList)));
	NAME_D3D12_OBJECT(_commandList);
	CHECK(_commandList->Close());

	CHECK(_d3dDevice->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&_drawFence)));
	_drawFenceEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
}

RenderD3D12::~RenderD3D12()
{
	CloseHandle(_drawFenceEvent);
}

void RenderD3D12::SetScissor(const RectRB &rect)
{
	Flush();
	_scissor = rect;
}

void RenderD3D12::SetViewport(const RectRB &rect)
{
	Flush();
	_viewport = rect;
}

void RenderD3D12::SetTransform(vec2d offset, float scale)
{
	Flush();
	_scale = scale;
	_offset = -offset;
}

void RenderD3D12::Begin(ID3D12Resource* rt, D3D12_CPU_DESCRIPTOR_HANDLE rtv, int displayWidth, int displayHeight, DisplayOrientation displayOrientation)
{
	CHECK(_commandAllocator->Reset());
	CHECK(_commandList->Reset(_commandAllocator.Get(), nullptr));

	_windowWidth = displayWidth;
	_windowHeight = displayHeight;
	_rtv = rtv;

	D3D12_RESOURCE_BARRIER presentResourceBarrier = {};
	presentResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentResourceBarrier.Transition.pResource = rt;
	presentResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
	presentResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &presentResourceBarrier);

	_commandList->ClearRenderTargetView(rtv, DirectX::XMVECTORF32{ 0, 0, 0, _ambient }, 0, nullptr);
}

void WaitForGPU(ID3D12Device* device, ID3D12CommandQueue* commandQueue)
{
	ComPtr<ID3D12Fence> fence;
	CHECK(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
	NAME_D3D12_OBJECT(fence);

	HANDLE waitCompleteEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
	CHECK(fence->SetEventOnCompletion(1, waitCompleteEvent));

	CHECK(commandQueue->Signal(fence.Get(), 1));

	WaitForSingleObject(waitCompleteEvent, INFINITE);
	CloseHandle(waitCompleteEvent);
}

void RenderD3D12::End(ID3D12Resource* rt)
{
	Flush();
	_curtex = nullptr;

	// Indicate that the render target will now be used to present when the command list is done executing.
	D3D12_RESOURCE_BARRIER presentResourceBarrier = {};
	presentResourceBarrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	presentResourceBarrier.Transition.pResource = rt;
	presentResourceBarrier.Transition.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
	presentResourceBarrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PRESENT;
	presentResourceBarrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
	_commandList->ResourceBarrier(1, &presentResourceBarrier);

	CHECK(_commandList->Close());
	ID3D12CommandList* commandLists[] = { _commandList.Get() }; // needed to cast to ID3D12CommandList
	_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	_rtv = {};

	WaitForGPU(_d3dDevice.Get(), _commandQueue.Get());
}

void RenderD3D12::SetMode(const RenderMode mode)
{
	Flush();
	_mode = mode;
}

bool RenderD3D12::TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter)
{
	assert(img.stride > 0);
	assert(img.bpp == 32);

	TextureData texture;

	D3D12_RESOURCE_DESC textureDesc = {};
	textureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	textureDesc.Width = img.width;
	textureDesc.Height = img.height;
	textureDesc.DepthOrArraySize = 1;
	textureDesc.MipLevels = 1;
	textureDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	textureDesc.SampleDesc.Count = 1;
	textureDesc.SampleDesc.Quality = 0;
	textureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

	CHECK(_d3dDevice->CreateCommittedResource(
		&c_defaultHeapProps,
		D3D12_HEAP_FLAG_NONE,
		&textureDesc,
		D3D12_RESOURCE_STATE_COPY_DEST,
		nullptr,
		IID_PPV_ARGS(&texture.resource)));
	NAME_D3D12_OBJECT(texture.resource);

	D3D12_DESCRIPTOR_HEAP_DESC srvHeapDesc = {};
	srvHeapDesc.NumDescriptors = 1;
	srvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	CHECK(_d3dDevice->CreateDescriptorHeap(&srvHeapDesc, IID_PPV_ARGS(&texture.srvHeap)));
	NAME_D3D12_OBJECT(texture.srvHeap);

	D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout;
	UINT numRows;
	UINT64 rowSizesInBytes;
	UINT64 totalBytes;
	_d3dDevice->GetCopyableFootprints(
		&textureDesc,
		0, // FirstSubresource
		1, // NumSubresources
		0, // BaseOffset
		&layout,
		&numRows,
		&rowSizesInBytes,
		&totalBytes);

	auto uploadBufferSize = img.width * img.height * (img.bpp / 8);

	D3D12_RESOURCE_DESC uploadBufferDesc = {};
	uploadBufferDesc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
	uploadBufferDesc.Width = uploadBufferSize;
	uploadBufferDesc.Height = 1;
	uploadBufferDesc.DepthOrArraySize = 1;
	uploadBufferDesc.MipLevels = 1;
	uploadBufferDesc.SampleDesc.Count = 1;
	uploadBufferDesc.SampleDesc.Quality = 0;
	uploadBufferDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;

	ComPtr<ID3D12Resource> uploadTextureBuffer;
	CHECK(_d3dDevice->CreateCommittedResource(&c_uploadHeapProps, D3D12_HEAP_FLAG_NONE, &uploadBufferDesc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&uploadTextureBuffer)));
	NAME_D3D12_OBJECT(uploadTextureBuffer);

	BYTE* mappedUploadBuffer;
	CHECK(uploadTextureBuffer->Map(0, NULL, reinterpret_cast<void**>(&mappedUploadBuffer)));

	void* destData = mappedUploadBuffer + layout.Offset;
	SIZE_T destRowPitch = layout.Footprint.RowPitch;
	SIZE_T destSlicePitch = (SIZE_T)layout.Footprint.RowPitch * numRows;

	D3D12_SUBRESOURCE_DATA subresourceData = {};
	subresourceData.pData = img.pixels;
	subresourceData.RowPitch = img.stride;
	subresourceData.SlicePitch = img.height * img.stride;

//  MemcpySubresource(&DestData, &subresourceData, (SIZE_T)rowSizesInBytes, numRows, layout.Footprint.Depth);
	for (UINT z = 0; z < layout.Footprint.Depth; ++z)
	{
		BYTE* destSlice = reinterpret_cast<BYTE*>(destData) + destSlicePitch * z;
		const BYTE* srcSlice = reinterpret_cast<const BYTE*>(subresourceData.pData) + subresourceData.SlicePitch * z;
		for (UINT y = 0; y < numRows; ++y)
		{
			memcpy(destSlice + destRowPitch * y, srcSlice + subresourceData.RowPitch * y, rowSizesInBytes);
		}
	}
// \MemcpySubresource

	uploadTextureBuffer->Unmap(0, NULL);

	D3D12_TEXTURE_COPY_LOCATION dstTextureCopyLocation = {};
	dstTextureCopyLocation.pResource = texture.resource.Get();
	dstTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;

	D3D12_TEXTURE_COPY_LOCATION srcTextureCopyLocation = {};
	srcTextureCopyLocation.pResource = uploadTextureBuffer.Get();
	srcTextureCopyLocation.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
	srcTextureCopyLocation.PlacedFootprint = layout;

	_commandList->CopyTextureRegion(&dstTextureCopyLocation, 0, 0, 0, &srcTextureCopyLocation, nullptr);

	D3D12_RESOURCE_BARRIER barrier = {};
	barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
	barrier.Transition.pResource = texture.resource.Get();
	barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
	barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
	_commandList->ResourceBarrier(1, &barrier);

	D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc = {};
	srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srvDesc.Format = textureDesc.Format;
	srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = 1;
	_d3dDevice->CreateShaderResourceView(texture.resource.Get(), &srvDesc, texture.srvHeap->GetCPUDescriptorHandleForHeapStart());

	texture.sampler = _samplerHeap->GetGPUDescriptorHandleForHeapStart();
	if (!magFilter)
		texture.sampler = OffsetDescriptorHandle(texture.sampler, _d3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_SAMPLER));

	CHECK(_commandList->Close());

	ID3D12CommandList* commandLists[] = { _commandList.Get() }; // needed to cast to ID3D12CommandList
	_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	// The command list can be reset anytime after ExecuteCommandList() is called.
	CHECK(_commandList->Reset(_commandAllocator.Get(), nullptr));

	// cannot release upload buffer while it's in use
	WaitForGPU(_d3dDevice.Get(), _commandQueue.Get());

	_textures.push_back(std::move(texture));
	tex.ptr = &_textures.back();

	return true;
}

void RenderD3D12::TexFree(DEV_TEXTURE tex)
{
	assert(_curtex != tex.ptr);
	for (auto it = _textures.begin(); it != _textures.end(); it++)
	{
		if (&*it == tex.ptr)
		{
			_textures.erase(it);
			return;
		}
	}
	assert(false);
}

void RenderD3D12::Flush()
{
	if( _iaSize > 0 && WIDTH(_viewport) > 0 && HEIGHT(_viewport) > 0 && WIDTH(_scissor) > 0 && HEIGHT(_scissor) > 0 )
	{
		assert(_curtex);
		_drawCount++;

		_commandList->SetGraphicsRootSignature(_rootSignature.Get());
		ID3D12DescriptorHeap* descriptorHeaps[] = { _samplerHeap.Get(), _curtex->srvHeap.Get() };
		_commandList->SetDescriptorHeaps(_countof(descriptorHeaps), descriptorHeaps);


		using namespace DirectX;

		MyConstants constantBufferData;
		float viewportHalfWidth = (float)WIDTH(_viewport) / 2;
		float viewportHalfHeight = (float)HEIGHT(_viewport) / 2;
		XMMATRIX view = XMMatrixTranslation(-_offset.x / _scale - viewportHalfWidth / _scale,
		                                    -_offset.y / _scale - viewportHalfHeight / _scale, 0.f) * XMMatrixScaling(_scale, _scale, 1.f);
		XMMATRIX proj = XMMatrixOrthographicLH(
			GetProjWidth(_viewport, _displayOrientation), -GetProjHeight(_viewport, _displayOrientation), -1.f, 1.f);
		XMMATRIX orientation = XMLoadFloat4x4(GetOrientationTransform(_displayOrientation));
		XMStoreFloat4x4(&constantBufferData.viewProj, XMMatrixTranspose(view * orientation * proj));

		// Constant buffer
		UINT constantBufferOffset = _ringAllocator.Allocate(sizeof(MyConstants), D3D12_CONSTANT_BUFFER_DATA_PLACEMENT_ALIGNMENT);

		// Vertex buffer
		UINT vertexBufferSizeInBytes = _vaSize * sizeof(MyVertex);
		UINT vertexBufferOffset = _ringAllocator.Allocate(vertexBufferSizeInBytes, 1 /* alignment */);

		// Index buffer
		UINT indexBufferSizeInBytes = _iaSize * sizeof(_indexArray[0]);
		UINT indexBufferOffset = _ringAllocator.Allocate(indexBufferSizeInBytes, sizeof(_indexArray[0]) /* alignment */);

		auto overlappedFenceValue = _ringAllocator.CommitFreeOverlapped(_drawCount /* fenceValue */);
		if (overlappedFenceValue > _drawFence->GetCompletedValue())
		{
			CHECK(_drawFence->SetEventOnCompletion(overlappedFenceValue, _drawFenceEvent));
			WaitForSingleObject(_drawFenceEvent, INFINITE);
		}

		// Copy all buffers data
		memcpy(_mappedRingBuffer + constantBufferOffset, &constantBufferData, sizeof(MyConstants));
		memcpy(_mappedRingBuffer + vertexBufferOffset, _vertexArray, vertexBufferSizeInBytes);
		memcpy(_mappedRingBuffer + indexBufferOffset, _indexArray, indexBufferSizeInBytes);

		auto ringBufferGPUVirtualAddress = _ringBuffer->GetGPUVirtualAddress();
		_commandList->SetGraphicsRootConstantBufferView(RootParameterCBV, ringBufferGPUVirtualAddress + constantBufferOffset);

		D3D12_VERTEX_BUFFER_VIEW vertexBufferView;
		vertexBufferView.BufferLocation = ringBufferGPUVirtualAddress + vertexBufferOffset;
		vertexBufferView.SizeInBytes = vertexBufferSizeInBytes;
		vertexBufferView.StrideInBytes = sizeof(MyVertex);
		_commandList->IASetVertexBuffers(0, 1, &vertexBufferView);

		D3D12_INDEX_BUFFER_VIEW indexBufferView;
		indexBufferView.BufferLocation = ringBufferGPUVirtualAddress + indexBufferOffset;
		indexBufferView.SizeInBytes = indexBufferSizeInBytes;
		indexBufferView.Format = DXGI_FORMAT_R16_UINT;
		_commandList->IASetIndexBuffer(&indexBufferView);

		_commandList->SetGraphicsRootDescriptorTable(RootParameterSRV, _curtex->srvHeap->GetGPUDescriptorHandleForHeapStart());
		_commandList->SetGraphicsRootDescriptorTable(RootParameterSampler, _curtex->sampler);
		switch (_mode)
		{
		default: assert(false);
		case RM_LIGHT: _commandList->SetPipelineState(_pipelineStateLight.Get()); break;
		case RM_WORLD: _commandList->SetPipelineState(_pipelineStateWorld.Get()); break;
		case RM_INTERFACE: _commandList->SetPipelineState(_pipelineStateUI.Get()); break;
		}
		_commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
		_commandList->RSSetScissorRects(1, &AsD3D12Rect(ApplyRectRotation(_scissor, _windowWidth, _windowHeight, _displayOrientation)));
		_commandList->RSSetViewports(1, &AsD3D12Viewport(ApplyRectRotation(_viewport, _windowWidth, _windowHeight, _displayOrientation)));
		_commandList->OMSetRenderTargets(1, &_rtv, false, nullptr);

		_commandList->DrawIndexedInstanced(_iaSize, 1, 0, 0, 0);

		CHECK(_commandList->Close());
		ID3D12CommandList* commandLists[] = { _commandList.Get() }; // needed to cast to ID3D12CommandList
		_commandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

		CHECK(_commandQueue->Signal(_drawFence.Get(), _drawCount));

		// The command list can be reset anytime after ExecuteCommandList() is called.
		CHECK(_commandList->Reset(_commandAllocator.Get(), nullptr));
	}
	_vaSize = 0;
	_iaSize = 0;
}

MyVertex* RenderD3D12::DrawQuad(DEV_TEXTURE tex)
{
	if( _curtex != tex.ptr )
	{
		Flush();
		_curtex = reinterpret_cast<TextureData *>(tex.ptr);
	}
	if( _vaSize > VERTEX_ARRAY_SIZE - 4 || _iaSize > INDEX_ARRAY_SIZE  - 6 )
	{
		Flush();
	}

	MyVertex *result = &_vertexArray[_vaSize];

	_indexArray[_iaSize]   = _vaSize;
	_indexArray[_iaSize+1] = _vaSize+1;
	_indexArray[_iaSize+2] = _vaSize+2;
	_indexArray[_iaSize+3] = _vaSize;
	_indexArray[_iaSize+4] = _vaSize+2;
	_indexArray[_iaSize+5] = _vaSize+3;

	_iaSize += 6;
	_vaSize += 4;

	return result;
}

MyVertex* RenderD3D12::DrawFan(unsigned int nEdges)
{
	assert(nEdges*3 < INDEX_ARRAY_SIZE);

	if( _vaSize + nEdges   > VERTEX_ARRAY_SIZE - 1 ||
		_iaSize + nEdges*3 > INDEX_ARRAY_SIZE )
	{
		Flush();
	}

	MyVertex *result = &_vertexArray[_vaSize];

	for( unsigned int i = 0; i < nEdges; ++i )
	{
		_indexArray[_iaSize + i*3    ] = _vaSize;
		_indexArray[_iaSize + i*3 + 1] = _vaSize + i + 1;
		_indexArray[_iaSize + i*3 + 2] = _vaSize + i + 2;
	}
	_indexArray[_iaSize + nEdges*3 - 1] = _vaSize + 1;

	_iaSize += nEdges*3;
	_vaSize += nEdges+1;

	return result;
}

void RenderD3D12::SetAmbient(float ambient)
{
	_ambient = ambient;
}


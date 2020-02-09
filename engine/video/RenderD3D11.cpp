#include "inc/video/RenderD3D11.h"
#include <d3d11_2.h>
#include <D3Dcompiler.h>
#include <DirectXMath.h>
#include <cstring>
#include <stdexcept>

#undef USE_MAP

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

// {FCAACEE2-F3A0-4FDB-BC50-139D23AF2C3B}
static const GUID s_sampler = 
{ 0xfcaacee2, 0xf3a0, 0x4fdb, { 0xbc, 0x50, 0x13, 0x9d, 0x23, 0xaf, 0x2c, 0x3b } };

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

static D3D11_RECT AsD3D11Rect(const RectRB &rect)
{
	return D3D11_RECT{ rect.left, rect.top, rect.right, rect.bottom };
}

static D3D11_VIEWPORT AsD3D11Viewport(const RectRB &rect)
{
	return D3D11_VIEWPORT
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

RenderD3D11::RenderD3D11(ID3D11RenderTargetView *&rtv, ID3D11DeviceContext2 *context)
	: _rtv(rtv)
	, _context(context)
{
	memset(_indexArray, 0, sizeof(_indexArray));
	memset(_vertexArray, 0, sizeof(_vertexArray));

	context->GetDevice(&_device);

	CHECK(_device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(MyConstants), D3D11_BIND_CONSTANT_BUFFER), nullptr, &_constantBuffer));

	CD3D11_BUFFER_DESC vertexBufferDesc(sizeof(_vertexArray), D3D11_BIND_VERTEX_BUFFER
#ifdef USE_MAP
		, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE
#endif
		);
	CHECK(_device->CreateBuffer(&vertexBufferDesc, nullptr, &_vertexBuffer));

	CD3D11_BUFFER_DESC indexBufferDesc(sizeof(_indexArray), D3D11_BIND_INDEX_BUFFER
#ifdef USE_MAP
		, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE
#endif
		);
	CHECK(_device->CreateBuffer(&indexBufferDesc, nullptr, &_indexBuffer));

	CD3D11_SAMPLER_DESC samplerDesc((CD3D11_DEFAULT()));
	samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	CHECK(_device->CreateSamplerState(&samplerDesc, &_samplerLinear));

	samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
	CHECK(_device->CreateSamplerState(&samplerDesc, &_samplerPoint));

	CD3D11_BLEND_DESC blendDescUI((CD3D11_DEFAULT()));
	blendDescUI.RenderTarget[0].BlendEnable = TRUE;
	blendDescUI.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
	blendDescUI.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDescUI.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
	CHECK(_device->CreateBlendState(&blendDescUI, &_blendStateUI));

	CD3D11_BLEND_DESC blendDescWorld((CD3D11_DEFAULT()));
	blendDescWorld.RenderTarget[0].BlendEnable = TRUE;
	blendDescWorld.RenderTarget[0].SrcBlend = D3D11_BLEND_DEST_ALPHA;
	blendDescWorld.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
	blendDescWorld.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_RED | D3D11_COLOR_WRITE_ENABLE_GREEN | D3D11_COLOR_WRITE_ENABLE_BLUE;
	CHECK(_device->CreateBlendState(&blendDescWorld, &_blendStateWorld));

	CD3D11_BLEND_DESC blendDescLight((CD3D11_DEFAULT()));
	blendDescLight.RenderTarget[0].BlendEnable = TRUE;
	blendDescLight.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_SRC_ALPHA;
	blendDescLight.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_ONE;
	blendDescLight.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALPHA;
	CHECK(_device->CreateBlendState(&blendDescLight, &_blendStateLight));

	CD3D11_RASTERIZER_DESC rasterizerDesc((CD3D11_DEFAULT()));
	rasterizerDesc.ScissorEnable = TRUE;
	CHECK(_device->CreateRasterizerState(&rasterizerDesc, &_rasterizerState));

	Microsoft::WRL::ComPtr<ID3DBlob> code;
	Microsoft::WRL::ComPtr<ID3DBlob> log;

	// Vertex shader & input layout
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
		code.ReleaseAndGetAddressOf(),  // [out] ppCode
		log.ReleaseAndGetAddressOf()    // [out] ppErrorMsgs
		)))
	{
		const char *msg = log ? (const char*)log->GetBufferPointer() : "Shader compilation failed";
		throw std::runtime_error(msg);
	}

	D3D11_INPUT_ELEMENT_DESC inputElementDescs[] =
	{
	//SemanticName SemanticIndex Format InputSlot AlignedByteOffset InputSlotClass InstanceDataStepRate
		{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "COLOR",    0, DXGI_FORMAT_R8G8B8A8_UNORM,  0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT,    0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0 },
	};
	CHECK(_device->CreateInputLayout(
		inputElementDescs,
		sizeof(inputElementDescs) / sizeof(inputElementDescs[0]),
		code->GetBufferPointer(),
		code->GetBufferSize(),
		&_inputLayout));

	CHECK(_device->CreateVertexShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &_vertexShader));

	// Pixel shaders
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
		code.ReleaseAndGetAddressOf(),  // [out] ppCode
		log.ReleaseAndGetAddressOf()    // [out] ppErrorMsgs
		)))
	{
		const char *msg = log ? (const char*)log->GetBufferPointer() : "Shader compilation failed";
		throw std::runtime_error(msg);
	}
	CHECK(_device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &_pixelShaderColor));

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
		code.ReleaseAndGetAddressOf(),  // [out] ppCode
		log.ReleaseAndGetAddressOf()    // [out] ppErrorMsgs
		)))
	{
		const char *msg = log ? (const char*)log->GetBufferPointer() : "Shader compilation failed";
		throw std::runtime_error(msg);
	}
	CHECK(_device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &_pixelShaderLight));
}

RenderD3D11::~RenderD3D11()
{
}

void RenderD3D11::SetScissor(const RectRB &rect)
{
	Flush();
	_context->RSSetScissorRects(1, &AsD3D11Rect(ApplyRectRotation(rect, _windowWidth, _windowHeight, _displayOrientation)));
}

void RenderD3D11::SetViewport(const RectRB &rect)
{
	Flush();
	_viewport = rect;
	_context->RSSetViewports(1, &AsD3D11Viewport(ApplyRectRotation(_viewport, _windowWidth, _windowHeight, _displayOrientation)));
}

void RenderD3D11::SetTransform(vec2d offset, float scale)
{
	Flush();
	_scale = scale;
	_offset = -offset;
}

void RenderD3D11::Begin(unsigned int displayWidth, unsigned int displayHeight, DisplayOrientation displayOrientation)
{
	_windowWidth = displayWidth;
	_windowHeight = displayHeight;

	ID3D11RenderTargetView *const targets[1] = { _rtv };
	_context->OMSetRenderTargets(1, targets, nullptr);
	_context->DiscardView(targets[0]);
	_context->ClearRenderTargetView(targets[0], DirectX::XMVECTORF32{ 0, 0, 0, _ambient });

	UINT stride = sizeof(MyVertex);
	UINT offset = 0;
	_context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	_context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	_context->IASetInputLayout(_inputLayout.Get());
	_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	_context->VSSetConstantBuffers(0, 1, _constantBuffer.GetAddressOf());
	_context->RSSetState(_rasterizerState.Get());
}

void RenderD3D11::End()
{
	Flush();
}

void RenderD3D11::SetMode(const RenderMode mode)
{
	Flush();

	switch( mode )
	{
	case RM_LIGHT:
		//glClearColor(0, 0, 0, _ambient);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
		//glClear(GL_COLOR_BUFFER_BIT);
		_context->PSSetShader(_pixelShaderLight.Get(), nullptr, 0);
		_context->OMSetBlendState(_blendStateLight.Get(), nullptr, ~0U);
		break;

	case RM_WORLD:
		_context->PSSetShader(_pixelShaderColor.Get(), nullptr, 0);
		_context->OMSetBlendState(_blendStateWorld.Get(), nullptr, ~0U);
		break;

	case RM_INTERFACE:
		_context->PSSetShader(_pixelShaderColor.Get(), nullptr, 0);
		_context->OMSetBlendState(_blendStateUI.Get(), nullptr, ~0U);
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderD3D11::TexCreate(DEV_TEXTURE &tex, ImageView img, bool magFilter)
{
	assert(img.stride > 0);
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R8G8B8A8_UNORM, img.width, img.height, 1, 1);
	D3D11_SUBRESOURCE_DATA data{ img.pixels, static_cast<UINT>(img.stride), 0 };
	CHECK(_device->CreateTexture2D(&desc, 32 == img.bpp ? &data : nullptr, &texture));
	CHECK(_device->CreateShaderResourceView(texture.Get(), nullptr, &srv));
	CHECK(srv->SetPrivateData(s_sampler, sizeof(void*), magFilter ? _samplerLinear.GetAddressOf() : _samplerPoint.GetAddressOf()));
	tex.ptr = srv.Detach();
	return true;
}

void RenderD3D11::TexFree(DEV_TEXTURE tex)
{
	((ID3D11ShaderResourceView*)tex.ptr)->Release();
}

void RenderD3D11::Flush()
{
	if( _iaSize > 0 && WIDTH(_viewport) > 0 && HEIGHT(_viewport) > 0 )
	{
		using namespace DirectX;

		MyConstants constantBufferData;
		float viewportHalfWidth = (float)WIDTH(_viewport) / 2;
		float viewportHalfHeight = (float)HEIGHT(_viewport) / 2;
		XMMATRIX view = XMMatrixTranslation(-_offset.x / _scale - viewportHalfWidth / _scale,
		                                    -_offset.y / _scale - viewportHalfHeight / _scale, 0.f)
			* XMMatrixScaling(_scale, _scale, 1.f);
		XMMATRIX proj = XMMatrixOrthographicLH(
			GetProjWidth(_viewport, _displayOrientation), -GetProjHeight(_viewport, _displayOrientation), -1.f, 1.f);
		XMMATRIX orientation = XMLoadFloat4x4(GetOrientationTransform(_displayOrientation));
		XMStoreFloat4x4(&constantBufferData.viewProj, XMMatrixTranspose(view * orientation * proj));
		_context->UpdateSubresource(_constantBuffer.Get(), 0, nullptr, &constantBufferData, 0, 0);

#ifdef USE_MAP
		D3D11_MAPPED_SUBRESOURCE mapped;

		CHECK(_context->Map(_vertexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
		memcpy(mapped.pData, &_vertexArray, sizeof(_vertexArray[0]) * _vaSize);
		_context->Unmap(_vertexBuffer.Get(), 0);

		CHECK(_context->Map(_indexBuffer.Get(), 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped));
		memcpy(mapped.pData, &_indexArray, sizeof(_indexArray[0]) * _iaSize);
		_context->Unmap(_indexBuffer.Get(), 0);
#else
		_context->UpdateSubresource(_vertexBuffer.Get(), 0, nullptr, &_vertexArray, 0, 0);
		_context->UpdateSubresource(_indexBuffer.Get(), 0, nullptr, &_indexArray, 0, 0);
#endif
		_context->DrawIndexed(_iaSize, 0, 0);
	}
	_vaSize = 0;
	_iaSize = 0;
}

MyVertex* RenderD3D11::DrawQuad(DEV_TEXTURE tex)
{
	if( _curtex != tex.ptr )
	{
		Flush();
		_curtex = tex.ptr;

		auto srv = (ID3D11ShaderResourceView*)tex.ptr;

		ID3D11SamplerState* samplerNoRef = nullptr;
		UINT dataSize = sizeof(samplerNoRef);
		CHECK(srv->GetPrivateData(s_sampler, &dataSize, &samplerNoRef));

		_context->PSSetShaderResources(0, 1, &srv);
		_context->PSSetSamplers(0, 1, &samplerNoRef);
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

MyVertex* RenderD3D11::DrawFan(unsigned int nEdges)
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

void RenderD3D11::DrawLines(const MyLine *lines, size_t count)
{
	// todo: do something
}

void RenderD3D11::SetAmbient(float ambient)
{
	_ambient = ambient;
}


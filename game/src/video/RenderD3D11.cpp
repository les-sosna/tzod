#include "inc/video/RenderD3D11.h"
#include "inc/video/RenderBase.h"
#include <wrl/client.h>
#include <d3d11.h>
#include <D3Dcompiler.h>
#include <cstring>
#include <stdexcept>

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048

struct MyConstants
{
	float scaleX;
	float scaleY;
	float offsetX;
	float offsetY;
};


class RenderD3D11 : public IRender
{
public:
	RenderD3D11(ID3D11DeviceContext *context);
	~RenderD3D11() override;

	// IRender
	void OnResizeWnd(unsigned int width, unsigned int height) override;

	void SetViewport(const RectRB *rect) override;
	void SetScissor(const RectRB *rect) override;
	void Camera(const RectRB *vp, float x, float y, float scale) override;

	int GetWidth() const override;
	int GetHeight() const override;

	void Begin(void) override;
	void End(void) override;
	void SetMode (const RenderMode mode) override;

	void SetAmbient(float ambient) override;

	bool TexCreate(DEV_TEXTURE &tex, const Image &img) override;
	void TexFree(DEV_TEXTURE tex) override;

	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;

	void DrawLines(const MyLine *lines, size_t count) override;

private:
	void Flush();
	int GetViewportWidth() const;
	int GetViewportHeight() const;


	Microsoft::WRL::ComPtr<ID3D11Device>        _device;
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> _context;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        _constantBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        _vertexBuffer;
	Microsoft::WRL::ComPtr<ID3D11Buffer>        _indexBuffer;
	//	Microsoft::WRL::ComPtr<ID3D11SamplerState>  _samplerState;
	Microsoft::WRL::ComPtr<ID3D11InputLayout>   _inputLayout;
	Microsoft::WRL::ComPtr<ID3D11VertexShader>  _vertexShader;
	Microsoft::WRL::ComPtr<ID3D11PixelShader>   _pixelShader;

	MyConstants _constantBufferData;

	int _windowWidth;
	int _windowHeight;
	RectRB _rtViewport;

	void* _curtex;
	float _ambient;

	UINT16 _indexArray[INDEX_ARRAY_SIZE];
	MyVertex _vertexArray[VERTEX_ARRAY_SIZE];

	unsigned int _vaSize;      // number of filled elements in _vertexArray
	unsigned int _iaSize;      // number of filled elements in _indexArray

	RenderMode  _mode;

//	GlesProgram _program;
//	GLuint _offset;
//	GLuint _scale;
//	GLuint _sampler;

};

///////////////////////////////////////////////////////////////////////////////

static const char s_vertexShader[] =
R"(
cbuffer MyConstantBuffer : register(b0)
{
	float scaleX;
	float scaleY;
	float offsetX;
	float offsetY;
};

struct VertexShaderInput
{
	float3 pos : POSITION;
	float4 color : COLOR;
	float2 tex : TEXCOORD;
};

struct PixelShaderInput
{
	float4 pos : SV_POSITION;
	float4 color : COLOR;
	float2 tex : TEXCOORD;
};

PixelShaderInput main(VertexShaderInput input)
{
	PixelShaderInput output;
	float4 pos = float4(input.pos, 1.0f);

	pos = mul(pos, model);
	pos = mul(pos, view);
	pos = mul(pos, projection);
	output.pos = pos;

	output.color = input.color;
	output.tex = input.tex;

	return output;
}
)";

static const char s_pixelShader[] =
R"(
    varying lowp vec2 texCoord;
    varying lowp vec4 color;
    uniform sampler2D s_tex;
    void main()
    {
        gl_FragColor = texture2D(s_tex, texCoord) * color;
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

///////////////////////////////////////////////////////////////////////////////

RenderD3D11::RenderD3D11(ID3D11DeviceContext *context)
	: _context(context)
	, _windowWidth(0)
	, _windowHeight(0)
//    , _curtex(-1)
	, _vaSize(0)
	, _iaSize(0)
	, _curtex(nullptr)
	//, _program(s_vertexShader, s_fragmentShader, s_bindings)
	//, _offset(_program.GetUniformLocation("vOffset"))
	//, _scale(_program.GetUniformLocation("vScale"))
	//, _sampler(_program.GetUniformLocation("s_tex"))
{
	memset(_indexArray, 0, sizeof(_indexArray));
	memset(_vertexArray, 0, sizeof(_vertexArray));

	context->GetDevice(&_device);

	CHECK(_device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(MyConstants), D3D11_BIND_CONSTANT_BUFFER), nullptr, &_constantBuffer));
	CHECK(_device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(_vertexArray), D3D11_BIND_VERTEX_BUFFER), nullptr, &_vertexBuffer));
	CHECK(_device->CreateBuffer(&CD3D11_BUFFER_DESC(sizeof(_indexArray), D3D11_BIND_INDEX_BUFFER), nullptr, &_indexBuffer));

//	CD3D11_SAMPLER_DESC samplerDesc;
//	CHECK(_device->CreateSamplerState(&samplerDesc, _samplerState));

	Microsoft::WRL::ComPtr<ID3DBlob> code;
	Microsoft::WRL::ComPtr<ID3DBlob> log;

	// Vertex shader & input layout
	if (FAILED(D3DCompile(
		s_vertexShader,                 // pSrcData
		sizeof(s_vertexShader),         // SrcDataSize
		"vs",                           // pSourceName
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
		throw std::runtime_error(log ? (const char*)log->GetBufferPointer() : "Shader compilation failed");
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

	// Pixel shader
	if (FAILED(D3DCompile(
		s_pixelShader,                  // pSrcData
		sizeof(s_pixelShader),          // SrcDataSize
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
		throw std::runtime_error(log ? (const char*)log->GetBufferPointer() : "Shader compilation failed");
	}

	CHECK(_device->CreatePixelShader(code->GetBufferPointer(), code->GetBufferSize(), nullptr, &_pixelShader));
}

RenderD3D11::~RenderD3D11()
{
}

void RenderD3D11::OnResizeWnd(unsigned int width, unsigned int height)
{
	_windowWidth = (int) width;
	_windowHeight = (int) height;
	SetViewport(nullptr);
	SetScissor(nullptr);
}

void RenderD3D11::SetScissor(const RectRB *rect)
{
	Flush();
	if( rect )
	{
		D3D11_RECT rect11 = { rect->left, rect->top, rect->right, rect->bottom };
		_context->RSSetScissorRects(1, &rect11);
	}
	else
	{
		_context->RSSetScissorRects(0, nullptr);
	}
}

void RenderD3D11::SetViewport(const RectRB *rect)
{
	Flush();

	if( rect )
	{
		_rtViewport = *rect;
		_constantBufferData.scaleX = 1.0f / WIDTH(*rect);
		_constantBufferData.scaleY = 1.0f / HEIGHT(*rect);

		D3D11_VIEWPORT viewport = { (FLOAT)rect->left, (FLOAT)rect->top, (FLOAT)WIDTH(*rect), (FLOAT)HEIGHT(*rect), 0.f, 1.f };
		_context->RSSetViewports(1, &viewport);
	}
	else
	{
		_rtViewport = RectRB{ 0, 0, _windowWidth, _windowHeight };
		_constantBufferData.scaleX = 1.0f / _windowWidth;
		_constantBufferData.scaleY = 1.0f / _windowHeight;

		// TODO: try removing viewport
		D3D11_VIEWPORT viewport = { 0.f, 0.f, (FLOAT)_windowWidth, (FLOAT)_windowHeight, 0.f, 1.f };
		_context->RSSetViewports(1, &viewport);
	}
}

void RenderD3D11::Camera(const RectRB *vp, float x, float y, float scale)
{
	SetViewport(vp);
	SetScissor(vp);

	// TODO: scale
	_constantBufferData.offsetX = vp ? (float) WIDTH(*vp) / 2 - x : .0f;
	_constantBufferData.offsetY = vp ? (float)HEIGHT(*vp) / 2 - y : .0f;
}

int RenderD3D11::GetWidth() const
{
	return _windowWidth;
}

int RenderD3D11::GetHeight() const
{
	return _windowHeight;
}

int RenderD3D11::GetViewportWidth() const
{
	return _rtViewport.right - _rtViewport.left;
}

int RenderD3D11::GetViewportHeight() const
{
	return _rtViewport.bottom - _rtViewport.top;
}

void RenderD3D11::Begin()
{
//    glEnable(GL_BLEND);

	UINT stride = sizeof(MyVertex);
	UINT offset = 0;
	_context->IASetVertexBuffers(0, 1, _vertexBuffer.GetAddressOf(), &stride, &offset);
	_context->IASetIndexBuffer(_indexBuffer.Get(), DXGI_FORMAT_R16_UINT, 0);
	_context->IASetInputLayout(_inputLayout.Get());
	_context->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	_context->VSSetShader(_vertexShader.Get(), nullptr, 0);
	_context->PSSetShader(_pixelShader.Get(), nullptr, 0);

//	glClearColor(0, 0, 0, _ambient);
//	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
//	glClear(GL_COLOR_BUFFER_BIT);
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
		//glDisable(GL_TEXTURE_2D);
		//glBlendFunc(GL_SRC_ALPHA, GL_ONE);
		//glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_TRUE);
		break;

	case RM_WORLD:
		//glEnable(GL_TEXTURE_2D);
		//glBlendFunc(GL_DST_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		break;

	case RM_INTERFACE:
		SetViewport(nullptr);
		Camera(nullptr, 0, 0, 1);
		//glEnable(GL_TEXTURE_2D);
		//glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		//glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_FALSE);
		//_context->OMSetBlendState(_hudBlendState.Get(), )
		break;

	default:
		assert(false);
	}

	_mode = mode;
}

bool RenderD3D11::TexCreate(DEV_TEXTURE &tex, const Image &img)
{
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView> srv;
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;
	CD3D11_TEXTURE2D_DESC desc(DXGI_FORMAT_R8G8B8A8_UNORM, img.GetWidth(), img.GetHeight(), 1, 1);
	D3D11_SUBRESOURCE_DATA data{ img.GetData(), img.GetWidth() * sizeof(SpriteColor), 0 };
	CHECK(_device->CreateTexture2D(&desc, 32 == img.GetBpp() ? &data : nullptr, &texture));
	CHECK(_device->CreateShaderResourceView(texture.Get(), nullptr, &srv));
	tex.ptr = srv.Detach();
	return true;
}

void RenderD3D11::TexFree(DEV_TEXTURE tex)
{
	((ID3D11ShaderResourceView*)tex.ptr)->Release();
}

void RenderD3D11::Flush()
{
	if( _iaSize )
	{
		_context->UpdateSubresource(_constantBuffer.Get(), 0, nullptr, &_constantBufferData, 0, 0);
		_context->UpdateSubresource(_vertexBuffer.Get(), 0, nullptr, &_vertexArray, 0, 0);
		_context->UpdateSubresource(_indexBuffer.Get(), 0, nullptr, &_indexArray, 0, 0);
		_context->DrawIndexed(_iaSize, 0, 0);
		_vaSize = 0;
		_iaSize = 0;
	}
}

MyVertex* RenderD3D11::DrawQuad(DEV_TEXTURE tex)
{
	if( _curtex != tex.ptr )
	{
		Flush();
		_curtex = tex.ptr;
		_context->PSSetShaderResources(0, 1, (ID3D11ShaderResourceView**)&tex.ptr);
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
	Flush();

//	glDisable(GL_TEXTURE_2D);
/*
	glBegin(GL_LINES);
	const MyLine *it = lines, *end = lines + count;
	for( ; it != end; ++it )
	{
		glColor4ub(it->color.rgba[3], it->color.rgba[2], it->color.rgba[1], it->color.rgba[0]);
		glVertex2fv((const float *) &it->begin);
		glVertex2fv((const float *) &it->end);
	}
	glEnd();
*/
	SetMode(_mode); // to enable texture
}

void RenderD3D11::SetAmbient(float ambient)
{
	_ambient = ambient;
}


//-----------------------------------------------------------------------------

std::unique_ptr<IRender> RenderCreateOpenGL(ID3D11DeviceContext *context)
{
	return std::unique_ptr<IRender>(new RenderD3D11(context));
}


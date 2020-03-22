#pragma once
#include "RenderBase.h"
#include <GLFW/glfw3.h>

#define VERTEX_ARRAY_SIZE   1024
#define  INDEX_ARRAY_SIZE   2048

class RenderOpenGL final
	: public IRender
{
public:
	RenderOpenGL();
	~RenderOpenGL() override;

	void Begin(unsigned int displayWidth, unsigned int displayHeight, DisplayOrientation displayOrientation);
	void End();

	// IRender
	void SetScissor(const RectRB& rect) override;
	void SetViewport(const RectRB& rect) override;
	void SetTransform(vec2d offset, float scale) override;
	void SetMode(const RenderMode mode) override;
	void SetAmbient(float ambient) override;
	bool TexCreate(DEV_TEXTURE& tex, ImageView img, bool magFilter) override;
	void TexFree(DEV_TEXTURE tex) override;
	MyVertex* DrawQuad(DEV_TEXTURE tex) override;
	MyVertex* DrawFan(unsigned int nEdges) override;
	void DrawLines(const MyLine* lines, size_t count) override;

private:
	void Flush();

	GLint _windowHeight = 0;
	GLuint _curtex = -1;
	float  _ambient = 0;

	GLushort _IndexArray[INDEX_ARRAY_SIZE];
	MyVertex _VertexArray[VERTEX_ARRAY_SIZE];

	unsigned int _vaSize = 0;      // number of filled elements in _VertexArray
	unsigned int _iaSize = 0;      // number of filled elements in _IndexArray

	RenderMode  _mode;
};

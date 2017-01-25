#pragma once

#include "RenderBase.h"
#include <stack>
#include <string>

class TextureManager;

enum enumAlignText {
	alignTextLT = 0, alignTextCT = 1, alignTextRT = 2,
	alignTextLC = 3, alignTextCC = 4, alignTextRC = 5,
	alignTextLB = 6, alignTextCB = 7, alignTextRB = 8,
};

class DrawingContext
{
public:
	DrawingContext(const TextureManager &tm, IRender &render, unsigned int width, unsigned int height);

	void PushClippingRect(RectRB rect);
	void PopClippingRect();

	void PushTransform(vec2d offset, float opacityCombined = 1);
	void PopTransform();

	RectRB GetVisibleRegion() const;

	void DrawSprite(const FRECT dst, size_t sprite, SpriteColor color, unsigned int frame);
	void DrawBorder(const FRECT &dst, size_t sprite, SpriteColor color, unsigned int frame);
	void DrawBitmapText(vec2d origin, float scale, size_t tex, SpriteColor color, const std::string &str, enumAlignText align = alignTextLT);
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, vec2d dir);
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, float width, float height, vec2d dir);
	void DrawIndicator(size_t tex, float x, float y, float value);
	void DrawLine(size_t tex, SpriteColor color, float x0, float y0, float x1, float y1, float phase);
	void DrawBackground(size_t tex, FRECT bounds) const;

	void DrawPointLight(float intensity, float radius, vec2d pos);
	void DrawSpotLight(float intensity, float radius, vec2d pos, vec2d dir, float offset, float aspect);
	void DrawDirectLight(float intensity, float radius, vec2d pos, vec2d dir, float length);

	void Camera(RectRB viewport, float x, float y, float scale);
	void SetAmbient(float ambient);
	void SetMode(const RenderMode mode);

private:
	struct Transform
	{
		vec2d offset;
		uint32_t opacity;
	};
	const TextureManager &_tm;
	IRender &_render;
	std::stack<RectRB> _clipStack;
	std::stack<Transform> _transformStack;
	RectRB _viewport;
	RenderMode _mode;
};

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
	DrawingContext(const TextureManager &tm, unsigned int width, unsigned int height);

	void PushClippingRect(const RectRB &rect);
	void PopClippingRect();

	void DrawSprite(const FRECT *dst, size_t sprite, SpriteColor color, unsigned int frame);
	void DrawBorder(const FRECT *dst, size_t sprite, SpriteColor color, unsigned int frame);
	void DrawBitmapText(float x, float y, size_t tex, SpriteColor color, const std::string &str, enumAlignText align = alignTextLT);
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, vec2d dir);
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, float x, float y, float width, float height, vec2d dir);
	void DrawIndicator(size_t tex, float x, float y, float value);
	void DrawLine(size_t tex, SpriteColor color, float x0, float y0, float x1, float y1, float phase);
	void DrawBackground(size_t tex, float sizeX, float sizeY) const;

	void DrawPointLight(float intensity, float radius, vec2d pos);
	void DrawSpotLight(float intensity, float radius, vec2d pos, vec2d dir, float offset, float aspect);
	void DrawDirectLight(float intensity, float radius, vec2d pos, vec2d dir, float length);

	void Camera(const RectRB &viewport, float x, float y, float scale);
	void SetAmbient(float ambient);
	void SetMode(const RenderMode mode);

private:
	const TextureManager &_tm;
	std::stack<RectRB> _clipStack;
	RectRB _viewport;
};
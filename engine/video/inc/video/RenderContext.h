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

class RenderContext final
{
public:
	RenderContext(const TextureManager &tm, IRender &render, unsigned int width, unsigned int height);

	void PushClippingRect(RectRB rect);
	void PopClippingRect();

	void PushTransform(vec2d offset, float opacityCombined = 1);
	void PushWorldTransform(vec2d offset, float scale);
	void PopTransform();

	FRECT GetVisibleRegion() const;
	float GetScale() const { return _scale; }

	void DrawSprite(const FRECT dst, size_t sprite, SpriteColor color, unsigned int frame);
	void DrawBorder(FRECT dst, size_t sprite, SpriteColor color, unsigned int frame);
	void DrawBitmapText(vec2d origin, float scale, size_t tex, SpriteColor color, std::string_view str, enumAlignText align = alignTextLT);
	void DrawBitmapText(FRECT rect, float scale, size_t tex, SpriteColor color, std::string_view str, enumAlignText align = alignTextLT);
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, vec2d pos, vec2d dir);
	void DrawSprite(size_t tex, unsigned int frame, SpriteColor color, vec2d pos, float width, float height, vec2d dir);
	void DrawIndicator(size_t tex, vec2d pos, float value);
	void DrawLine(size_t tex, SpriteColor color, vec2d begin, vec2d end, float phase);
	void DrawBackground(size_t tex, FRECT bounds) const;

	void DrawPointLight(float intensity, float radius, vec2d pos);
	void DrawSpotLight(float intensity, float radius, vec2d pos, vec2d dir, float offset, float aspect);
	void DrawDirectLight(float intensity, float radius, vec2d pos, vec2d dir, float length);

	void SetAmbient(float ambient);
	void SetMode(const RenderMode mode);

private:
	struct Transform
	{
		vec2d offset;
		uint32_t opacity;
		bool hardware;
	};
	const TextureManager &_tm;
	IRender &_render;
	std::stack<RectRB> _clipStack;
	std::stack<Transform> _transformStack;
	Transform _currentTransform; // local copy of _transformStack.top
	float _scale = 1;
	RenderMode _mode;
};

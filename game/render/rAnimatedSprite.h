#pragma once
#include "inc/render/ObjectView.h"

#include <stddef.h>
#include <vector>

class TextureManager;

class R_AnimatedSprite : public ObjectRFunc
{
public:
	R_AnimatedSprite(TextureManager &tm, const char *tex, float frameRate);
	void Draw(const World &world, const GC_MovingObject &mo, RenderContext &rc) const override;

private:
	TextureManager &_tm;
	size_t _texId;
	float _frameRate;
};

class R_AnimatedSpriteSequence : public ObjectRFunc
{
public:
	R_AnimatedSpriteSequence(TextureManager& tm, const char* tex, float frameRate, std::vector<int> frames);
	void Draw(const World& world, const GC_MovingObject& mo, RenderContext& rc) const override;

private:
	size_t _texId;
	float _frameRate;
	std::vector<int> _frames;
};

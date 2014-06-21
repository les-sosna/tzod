#pragma once

#include <stddef.h>

class TextureManager;
struct IRender;

class Terrain
{
public:
	Terrain(TextureManager &tm);
	
	void Draw(IRender &render, float sizeX, float sizeY, bool editorMode) const;
	
private:
	TextureManager &_tm;
	size_t _texBack;
	size_t _texGrid;
	void DrawBackground(IRender &render, float sizeX, float sizeY, size_t tex) const;
};

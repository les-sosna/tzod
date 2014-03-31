// types.h

#pragma once

#include <cstdint>

struct FRECT
{
	float left;
	float top;
	float right;
	float bottom;
};

struct SpriteColor
{
	union {
		unsigned char rgba[4];
		uint32_t color;
		struct {
			unsigned char r,g,b,a;
		};
	};

	SpriteColor() {}
	SpriteColor(uint32_t c) : color(c) {}
};

typedef int ObjectType;
#define INVALID_OBJECT_TYPE (-1)

enum enumAlignText {
	alignTextLT = 0, alignTextCT = 1, alignTextRT = 2,
	alignTextLC = 3, alignTextCC = 4, alignTextRC = 5,
	alignTextLB = 6, alignTextCB = 7, alignTextRB = 8,
};

typedef float AIPRIORITY;

// end of file

// types.h

typedef struct FRECT
{
	float left;
	float top;
	float right;
	float bottom;
} FRECT, *LPFRECT;

struct SpriteColor
{
	union {
		BYTE   rgba[4];
		DWORD  dwColor;
		struct {
			BYTE r,g,b,a;
		};
	};

	SpriteColor() {}
	SpriteColor(DWORD c) : dwColor(c) {}
};

typedef int ObjectType;
#define INVALID_OBJECT_TYPE (-1)

enum enumAlignText {
	alignTextLT = 0, alignTextCT = 1, alignTextRT = 2,
	alignTextLC = 3, alignTextCC = 4, alignTextRC = 5,
	alignTextLB = 6, alignTextCB = 7, alignTextRB = 8,
};

typedef std::string            string_t;
typedef std::ostringstream     ostrstream_t;
typedef std::istringstream     istrstream_t;

typedef float AIPRIORITY;

// end of file

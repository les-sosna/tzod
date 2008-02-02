// 2dSprite.h
/////////////////////////////////////////

#pragma once

#include "Actor.h"

/////////////////////////////////////////////////

class TextureCache
{
	friend class GC_2dSprite;
	float  width;
	float  height;
	vec2d  pivot;
	size_t texture;
	SpriteColor color;

public:
	TextureCache(const char *name);
};

///////////////////////////////////////////
// flags
#define GC_FLAG_2DSPRITE_VISIBLE               (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_2DSPRITE_INGRIDSET             (GC_FLAG_ACTOR_ << 1)
#define GC_FLAG_2DSPRITE_DROPSHADOW            (GC_FLAG_ACTOR_ << 2)
#define GC_FLAG_2DSPRITE_CENTERPIVOT           (GC_FLAG_ACTOR_ << 3)
#define GC_FLAG_2DSPRITE_                      (GC_FLAG_ACTOR_ << 4)

class GC_2dSprite : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_2dSprite);

private:
	FRECT  _frameRect;  // texture coords
	float  _width;
	float  _height;
	float  _rotation;

	SpriteColor  _color;

	vec2d  _pivot;      // локальные координаты центра спрайта
	size_t _texId;

private:
	int    _frame;      // value -1 means custom _rtFrame

public:
	inline int   GetFrameCount()   const;
	inline int   GetCurrentFrame() const { return _frame; }
	inline const vec2d& GetPivot() const { return _pivot; }
	inline void  GetGlobalRect(FRECT &rect) const
	{
		rect.left   = GetPos().x - _pivot.x;
		rect.top    = GetPos().y - _pivot.y;
		rect.right  = rect.left + GetSpriteWidth();
		rect.bottom = rect.top  + GetSpriteHeight();
	}
	inline void  GetLocalRect(FRECT &rect) const
	{
		_ASSERT(_texId);
		rect.left   = -_pivot.x;
		rect.top    = -_pivot.y;
		rect.right  = rect.left + GetSpriteWidth();
		rect.bottom = rect.top + GetSpriteHeight();
	}

	void SetTexture(const char *name);
	void SetTexture(const TextureCache &tc);
	void UpdateTexture();

	void Resize(float width, float height)
	{
		_width = width;
		_height = height;
	}

	void SetFrame(int nFrame);
	void ModifyFrameBounds(const LPFRECT lpFrame);
	void SetPivot(const vec2d &pivot);
	void SetPivot(float x, float y);
	void CenterPivot();

	virtual void MoveTo(const vec2d &pos);

	inline void SetRotation(float a) { _rotation = a; }
	inline void SetOpacity(float x) { SetOpacity1i( int(x * 255.0f) ); }
	inline void SetOpacity1i(int x) { _color.r = _color.g = _color.b = _color.a = x & 0xff; }
	inline void SetColor(BYTE r, BYTE g, BYTE b) { _color.r=r; _color.g=g; _color.b=b; }

	inline float GetRotation() const { return _rotation; }


	void SetShadow(bool bEnable)
	{
		bEnable?SetFlags(GC_FLAG_2DSPRITE_DROPSHADOW):ClearFlags(GC_FLAG_2DSPRITE_DROPSHADOW);
	}
	bool GetShadow() const
	{
		return CheckFlags(GC_FLAG_2DSPRITE_DROPSHADOW);
	}


private:
	OBJECT_LIST::iterator _globalZPos; // позиция в списке onscreen или z_globals

private:
	void UpdateCurrentZ();
	void SetZ_current(enumZOrder z);
	enumZOrder _zOrderCurrent;
	enumZOrder _zOrderPrefered;

public:
	inline float GetSpriteWidth() const { return _width; }
	inline float GetSpriteHeight() const { return _height; }

public:
	void SetGridSet(bool bGridSet);
	void SetZ(enumZOrder z);
	enumZOrder GetZ() const;
	void Show(bool bShow);


	inline bool IsVisible()  { return CheckFlags(GC_FLAG_2DSPRITE_VISIBLE); }

public:
	GC_2dSprite();
	GC_2dSprite(FromFile);
	virtual ~GC_2dSprite();

	virtual void Serialize(SaveFile &f);

	virtual void Kill();
	virtual void Draw();
};

/////////////////////////////////////////////////////////////

class GC_UserSprite : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_UserSprite);

public:
	GC_UserSprite();
	GC_UserSprite(FromFile);

	virtual bool IsSaved() const { return true; }
};


///////////////////////////////////////////////////////////////////////////////
// end of file

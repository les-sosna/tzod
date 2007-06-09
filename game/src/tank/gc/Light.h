// Light.h

#pragma once

#include "Object.h"
#include "2dSprite.h"

///////////////////////////////////////////////////////////
// flags

#define GC_FLAG_LIGHT_ENABLED        (GC_FLAG_OBJECT_ << 0)


///////////////////////////////////////////////////////////////////////////////
// forward class declarations

class GC_UserSprite;


///////////////////////////////////////////////////////////////////////////////

class GC_Light : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Light);
	MemberOfGlobalList _memberOf;

public:
	enum enumLightType
	{
		LIGHT_POINT,
		LIGHT_SPOT,
		LIGHT_DIRECT,
	};

private:
	float  _timeout;
	float  _aspect;
	float  _offset;
	float  _angle;
	float  _radius;
	float  _intensity;
	enumLightType _type;

	SafePtr<GC_UserSprite> _lamp;

	static const int SINTABLE_SIZE = 32;
	static const int SINTABLE_MASK = 0x1f;
	static float _sintable[SINTABLE_SIZE];

public:
	GC_Light(enumLightType type);
	GC_Light(FromFile);
	virtual ~GC_Light();

	virtual bool IsSaved() { return true; }
	virtual void Serialize(SaveFile &f);

	void SetIntensity(float i)
	{
		_intensity = i;
	}
	void SetAspect(float a)
	{
		_aspect = a;
	}
	void SetRadius(float r)
	{
		if( LIGHT_DIRECT == _type )
			_offset = r;
		else
			_radius = r;
	}
	void SetAngle(float a)
	{
		_angle = a;
	}
	void SetOffset(float o)
	{
		_ASSERT(LIGHT_DIRECT != _type);
		_offset = o;
	}
	void SetLength(float l)
	{
		_ASSERT(LIGHT_DIRECT == _type);
		_radius = l;
	}
	float GetLength() const
	{
		_ASSERT(LIGHT_DIRECT == _type);
		return _radius;
	}

	void Enable(bool bEnable);


	float GetRadius() const
	{
		if( LIGHT_DIRECT == _type )
			return _offset;
		else
			return _radius;
	}

	float GetRenderRadius() const
	{
		if( LIGHT_SPOT == _type )
			return _offset + _radius;
		else
			return _radius;
	}


	void  SetTimeout(float t);
	float GetTimeout() const { return _timeout; }

	virtual void Kill();
	virtual void MoveTo(const vec2d &pos);

	virtual void TimeStepFixed(float dt);

public:
	virtual void Shine();
};


class GC_Spotlight : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Spotlight);
	SafePtr<GC_Light> _light;

	virtual SafePtr<PropertySet> GetProperties();

public:
	GC_Spotlight(float x, float y);
	GC_Spotlight(FromFile);
	virtual ~GC_Spotlight();

	virtual bool IsSaved() { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void Kill();
	virtual void MoveTo(const vec2d &pos);

	virtual void EditorAction();
	virtual void mapExchange(MapFile &f);
};


///////////////////////////////////////////////////////////////////////////////
// end of file

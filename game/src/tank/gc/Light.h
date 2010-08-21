// Light.h

#pragma once

#include "Object.h"
#include "2dSprite.h"

///////////////////////////////////////////////////////////
// flags

#define GC_FLAG_LIGHT_ACTIVE        (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_LIGHT_              (GC_FLAG_ACTOR_ << 1)

///////////////////////////////////////////////////////////////////////////////

class GC_Light : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Light);
	MemberOfGlobalList<LIST_lights> _memberOf;

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
	float  _radius;
	float  _intensity;
	enumLightType _type;
	vec2d  _lightDirection;

	SafePtr<GC_2dSprite> _lampSprite;

	static const int SINTABLE_SIZE = 32;
	static const int SINTABLE_MASK = 0x1f;
	static float _sintable[SINTABLE_SIZE];

public:
	GC_Light(enumLightType type);
	GC_Light(FromFile);
	virtual ~GC_Light();

	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);

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
	void SetLightDirection(const vec2d &d)
	{
		_lightDirection = d;
	}
	void SetOffset(float o)
	{
		assert(LIGHT_DIRECT != _type);
		_offset = o;
	}
	void SetLength(float l)
	{
		assert(LIGHT_DIRECT == _type);
		_radius = l;
	}
	float GetLength() const
	{
		assert(LIGHT_DIRECT == _type);
		return _radius;
	}

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

	bool IsActive() const { return CheckFlags(GC_FLAG_LIGHT_ACTIVE); }
	void SetActive(bool activate);

	virtual void Kill();
	virtual void MoveTo(const vec2d &pos);

	virtual void TimeStepFixed(float dt);

public:
	virtual void Shine() const;
	virtual void Update(); // handles changing day/night
};

///////////////////////////////////////////////////////////////////////////////

class GC_Spotlight : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Spotlight);
	SafePtr<GC_Light> _light;


protected:
	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propActive;
		ObjectProperty _propDir;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_Spotlight(float x, float y);
	GC_Spotlight(FromFile);
	virtual ~GC_Spotlight();

	virtual void Serialize(SaveFile &f);

	virtual void Kill();
	virtual void MoveTo(const vec2d &pos);

	virtual void EditorAction();
	virtual void MapExchange(MapFile &f);
};


///////////////////////////////////////////////////////////////////////////////
// end of file

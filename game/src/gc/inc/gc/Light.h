#pragma once
#include "Actor.h"
#include "ObjPtr.h"

#define GC_FLAG_LIGHT_ACTIVE        (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_LIGHT_FADE          (GC_FLAG_ACTOR_ << 1)
#define GC_FLAG_LIGHT_              (GC_FLAG_ACTOR_ << 2)

class GC_Light : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Light);
    DECLARE_LIST_MEMBER(override);
    typedef GC_Actor base;

public:
	enum enumLightType
	{
		LIGHT_POINT,
		LIGHT_SPOT,
		LIGHT_DIRECT,
	};

private:
	float  _startTime;
	float  _timeout;
	float  _aspect;
	float  _offset;
	float  _radius;
	float  _intensity;
	enumLightType _type;
	vec2d  _lightDirection;

public:
	GC_Light(vec2d pos, enumLightType type);
	explicit GC_Light(FromFile);
	virtual ~GC_Light();

	enumLightType GetLightType() const { return _type; }

	void SetIntensity(float i) { _intensity = i; }
	float GetIntensity() const { return _intensity; }

	void SetAspect(float a) { _aspect = a; }
	float GetAspect() const { return _aspect; }

	void SetRadius(float r)
	{
		if( LIGHT_DIRECT == _type )
			_offset = r;
		else
			_radius = r;
	}
	void SetLightDirection(const vec2d &d) { _lightDirection = d; }
	vec2d GetLightDirection() const { return _lightDirection; }
	void SetOffset(float o)
	{
		assert(LIGHT_DIRECT != _type);
		_offset = o;
	}
	float GetOffset() const
	{
		assert(LIGHT_DIRECT != _type);
		return _offset;
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

	bool GetFade() const { return CheckFlags(GC_FLAG_LIGHT_FADE); }
	float GetStartTime() const { return _startTime; }

	void  SetTimeout(World &world, float t);
	float GetTimeout() const { return _timeout; }

	bool GetActive() const { return CheckFlags(GC_FLAG_LIGHT_ACTIVE); }
	void SetActive(bool activate);

    void Resume(World &world) override;
    void Serialize(World &world, SaveFile &f) override;
};

///////////////////////////////////////////////////////////////////////////////

#define GC_FLAG_SPOTLIGHT_ACTIVE    (GC_FLAG_ACTOR_ << 0)
#define GC_FLAG_SPOTLIGHT_          (GC_FLAG_ACTOR_ << 1)

class GC_Spotlight : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Spotlight);

public:
	explicit GC_Spotlight(vec2d pos);
	explicit GC_Spotlight(FromFile);
	virtual ~GC_Spotlight();

	// GC_Actor
    void MoveTo(World &world, const vec2d &pos) override;

	// GC_Object
    void Init(World &world) override;
    void Kill(World &world) override;
    void MapExchange(MapFile &f) override;
    void Serialize(World &world, SaveFile &f) override;

protected:
	class MyPropertySet : public GC_Actor::MyPropertySet
	{
		typedef GC_Actor::MyPropertySet BASE;
		ObjectProperty _propActive;
		ObjectProperty _propDir;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
    PropertySet* NewPropertySet() override;

private:
	ObjPtr<GC_Light> _light;
};

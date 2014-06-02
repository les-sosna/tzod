// indicators.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

class GC_Vehicle;

///////////////////////////////////////////////////////////////////////////////

class GC_SpawnPoint : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_SpawnPoint);
    typedef GC_2dSprite base;

public:
	int   _team;    // 0 - no team

protected:
	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propTeam;
		ObjectProperty _propDir;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();


public:
    DECLARE_MEMBER_OF();
	GC_SpawnPoint(World &world);
	GC_SpawnPoint(FromFile);

	virtual void Serialize(World &world, SaveFile &f);

	virtual void Draw(bool editorMode) const;
	virtual void MapExchange(World &world, MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_HideLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HideLabel);

public:
	GC_HideLabel(World &world);
	GC_HideLabel(FromFile);

	virtual void Draw(bool editorMode) const;
};

///////////////////////////////////////////////////////////////////////////////
// flags
#define GC_FLAG_INDICATOR_INVERSE      (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_INDICATOR_             (GC_FLAG_2DSPRITE_ << 1)

enum LOCATION
{
	LOCATION_TOP,
	LOCATION_BOTTOM,
};

class GC_IndicatorBar : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_IndicatorBar);
    typedef GC_2dSprite base;

	void OnParentKill(World &world, GC_Object *sender, void *param);
	void OnUpdatePosition(World &world, GC_Object *sender, void *param);

protected:
	ObjPtr<GC_2dSprite> _object;

	size_t _dwValue_offset;
	size_t _dwValueMax_offset;

	LOCATION _location;

public:
    DECLARE_MEMBER_OF();
	GC_IndicatorBar(World &world, const char *texture, GC_2dSprite *object, float *pValue, float *pValueMax, LOCATION location);
	GC_IndicatorBar(FromFile);

	// GC_2dSprite
	virtual void Draw(bool editorMode) const;

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);

public:
	void SetInverse(bool bInverse) { SetFlags(GC_FLAG_INDICATOR_INVERSE, bInverse); }
	static GC_IndicatorBar *FindIndicator(World &world, GC_2dSprite* object, LOCATION location);
};

///////////////////////////////////////////////////////////////////////////////

class GC_DamLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_DamLabel);
    typedef GC_2dSprite base;

private:
	float _phase;
	float _time;
	float _time_life;

public:
    DECLARE_MEMBER_OF();
	GC_DamLabel(World &world, GC_Vehicle *parent);
	GC_DamLabel(FromFile);
	virtual ~GC_DamLabel();

	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStepFloat(World &world, float dt);

	void Reset();

	void OnVehicleMove(World &world, GC_Object *sender, void *param);
};


// end of file

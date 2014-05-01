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
	MemberOfGlobalList<LIST_respawns> _memberOf;

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
		virtual void MyExchange(Level &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();


public:
	GC_SpawnPoint(Level &world, float x, float y);
	GC_SpawnPoint(FromFile);

	virtual void Serialize(Level &world, SaveFile &f);

	virtual void Draw(bool editorMode) const;
	virtual void EditorAction(Level &world);
	virtual void MapExchange(Level &world, MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_HideLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HideLabel);

public:
	GC_HideLabel(Level &world, float x, float y);
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
	MemberOfGlobalList<LIST_indicators> _memberOf;

	void OnParentKill(Level &world, GC_Object *sender, void *param);
	void OnUpdatePosition(Level &world, GC_Object *sender, void *param);

protected:
	ObjPtr<GC_2dSprite> _object;

	size_t _dwValue_offset;
	size_t _dwValueMax_offset;

	LOCATION _location;

public:
	GC_IndicatorBar(Level &world, const char *texture, GC_2dSprite *object, float *pValue, float *pValueMax, LOCATION location);
	GC_IndicatorBar(FromFile);

	// GC_2dSprite
	virtual void Draw(bool editorMode) const;

	// GC_Object
	virtual void Serialize(Level &world, SaveFile &f);

public:
	void SetInverse(bool bInverse) { SetFlags(GC_FLAG_INDICATOR_INVERSE, bInverse); }
	static GC_IndicatorBar *FindIndicator(Level &world, GC_2dSprite* object, LOCATION location);
};

///////////////////////////////////////////////////////////////////////////////

class GC_DamLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_DamLabel);

private:
	float _phase;
	float _time;
	float _time_life;

public:
	GC_DamLabel(Level &world, GC_Vehicle *parent);
	GC_DamLabel(FromFile);
	virtual ~GC_DamLabel();

	virtual void Serialize(Level &world, SaveFile &f);
	virtual void TimeStepFloat(Level &world, float dt);

	void Reset();

	void OnVehicleMove(Level &world, GC_Object *sender, void *param);
};


// end of file

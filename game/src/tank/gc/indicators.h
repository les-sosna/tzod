// indicators.h

#pragma once

#include "2dSprite.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

class GC_Text;
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
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();


public:
	GC_SpawnPoint(float x, float y);
	GC_SpawnPoint(FromFile);

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void Draw() const;
	virtual void EditorAction();
	virtual void mapExchange(MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_HideLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HideLabel);

public:
	GC_HideLabel(float x, float y);
	GC_HideLabel(FromFile);

	virtual bool IsSaved() const { return true; }
	virtual void Draw() const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Crosshair : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Crosshair);
public:
	enum enChStyle
	{
		CHS_SINGLE,
		CHS_DOUBLE
	};

private:
	float _time;

public:
	float     _angle;
	enChStyle _chStyle;

public:
	GC_Crosshair(enChStyle style);
	GC_Crosshair(FromFile);

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////

enum LOCATION
{
	LOCATION_TOP,
	LOCATION_BOTTOM,
};

class GC_IndicatorBar : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_IndicatorBar);
	MemberOfGlobalList<LIST_indicators> _memberOf;

protected:
	SafePtr<GC_2dSprite> _object;

	DWORD _dwValue_offset;
	DWORD _dwValueMax_offset;

	LOCATION _location;

	bool _bInverse;

public:
	GC_IndicatorBar(const char *texture, GC_2dSprite *object, float *pValue, float *pValueMax, LOCATION location);
	GC_IndicatorBar(FromFile);

	void OnParentKill(GC_Object *sender, void *param);
	void OnUpdateValue(GC_Object *sender, void *param);
	void OnUpdatePosition(GC_Object *sender, void *param);

//	virtual void Draw() const;

	// GC_Object
	virtual void Kill();
	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

public:
	void SetInverse(bool bInverse) { _bInverse = bInverse; };
	static GC_IndicatorBar *FindIndicator(GC_2dSprite* object, LOCATION location);
};

///////////////////////////////////////////////////////////////////////////////

class GC_VehicleVisualDummy;

class GC_DamLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_DamLabel);

private:
	float _rot;
	float _time;
	float _time_life;

public:
	GC_DamLabel(GC_VehicleVisualDummy *parent);
	GC_DamLabel(FromFile);
	virtual ~GC_DamLabel();

	virtual bool IsSaved() const { return true; }
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);

	void Reset();

	void OnVehicleMove(GC_Object *sender, void *param);
};


// end of file

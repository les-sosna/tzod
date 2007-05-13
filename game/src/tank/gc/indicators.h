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
	MemberOfGlobalList _memberOf;

public:
	int   _team;		// 0 - no team

public:
	GC_SpawnPoint(float x, float y);
	GC_SpawnPoint(FromFile);

	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	virtual void Draw();
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

	virtual bool IsSaved() { return true; };
	virtual void Draw();
};

///////////////////////////////////////////////////////////////////////////////
/*
class GC_Cursor : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Cursor);
protected:
	int _time_show;
	float _time_animation;
	SafePtr<GC_Text> _text;

public:
	GC_Cursor();
	GC_Cursor(FromFile) : GC_2dSprite(FromFile()) {};
	virtual void EndFrame();

	void Kill();
	void TimeStepFloat(float dt);
};

///////////////////////////////////////////////////////////////////////////////
*/

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
	float		_time;

public:
	float		_angle;
	enChStyle	_chStyle;

public:
	GC_Crosshair(enChStyle style);
	GC_Crosshair(FromFile);

	virtual bool IsSaved() { return true; };
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
	MemberOfGlobalList _memberOf;

protected:
	SafePtr<GC_2dSprite> _object;

	float _initial_width;

	DWORD _dwValue_offset;
	DWORD _dwValueMax_offset;

	LOCATION _location;

	bool _bInverse;

public:
	GC_IndicatorBar(const char *texture, GC_2dSprite* object, float *pValue, float *pValueMax, LOCATION location);
	GC_IndicatorBar(FromFile);

	virtual void Kill();

	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	void OnParentKill(GC_Object *sender, void *param);
	void OnUpdate(GC_Object *sender, void *param);

public:
	void SetInverse(bool bInverse) { _bInverse = bInverse; };
	static GC_IndicatorBar *FindIndicator(GC_2dSprite* object, LOCATION location);
};

///////////////////////////////////////////////////////////////////////////////

class GC_DamLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_DamLabel);

private:
	float _time;
	float _time_life;

	SafePtr<GC_Vehicle> _vehicle;

public:
	GC_DamLabel(GC_Vehicle *pVehicle);
	GC_DamLabel(FromFile);
	virtual ~GC_DamLabel();
	virtual void Kill();

	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	virtual void TimeStepFloat(float dt);
	virtual void Draw();

	void Reset();

	void OnVehicleMove(GC_Object *sender, void *param);
};


// end of file

// Trigger.h

#pragma once


#include "2dSprite.h"

#define GC_FLAG_TRIGGER_ACTIVE        (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_TRIGGER_ONLYVISIBLE   (GC_FLAG_2DSPRITE_ << 1)
#define GC_FLAG_TRIGGER_ONLYHUMAN     (GC_FLAG_2DSPRITE_ << 2)
#define GC_FLAG_TRIGGER_              (GC_FLAG_2DSPRITE_ << 3)

// forward declarations
class GC_Vehicle;


class GC_Trigger : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Trigger);

	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propActive;
		ObjectProperty _propTeam;
		ObjectProperty _propRadius;
		ObjectProperty _propRadiusDelta;
		ObjectProperty _propOnlyVisible;
		ObjectProperty _propOnlyHuman;
		ObjectProperty _propOnEnter;
		ObjectProperty _propOnLeave;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};

	virtual PropertySet* NewPropertySet();

	float    _radius;
	float    _radiusDelta;
	int      _team;
	string_t _onEnter;
	string_t _onLeave;

	SafePtr<GC_Vehicle> _veh;

	bool IsVisible(const GC_Vehicle *v);

public:
	GC_Trigger(float x, float y);
	GC_Trigger(FromFile);
	~GC_Trigger();

	virtual void mapExchange(MapFile &f);
	virtual void Serialize(SaveFile &f);
	virtual bool IsSaved() const { return true; }

	virtual void TimeStepFixed(float dt);
	virtual void Draw();
};

// end of file

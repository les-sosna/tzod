// Trigger.h

#pragma once


#include "2dSprite.h"

#define GC_FLAG_TRIGGER_ENABLED       (GC_FLAG_2DSPRITE_ << 0)
#define GC_FLAG_TRIGGER_ONLYVISIBLE   (GC_FLAG_2DSPRITE_ << 1)
#define GC_FLAG_TRIGGER_ONLYHUMAN     (GC_FLAG_2DSPRITE_ << 2)
#define GC_FLAG_TRIGGER_ACTIVATED     (GC_FLAG_2DSPRITE_ << 3)
#define GC_FLAG_TRIGGER_              (GC_FLAG_2DSPRITE_ << 4)

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
		virtual void MyExchange(World &world, bool applyToObject);
	};

	virtual PropertySet* NewPropertySet();

	float    _radius;
	float    _radiusDelta;
	int      _team;
	std::string _onEnter;
	std::string _onLeave;

	ObjPtr<GC_Vehicle> _veh;

	bool GetVisible(World &world, const GC_Vehicle *v) const;
	bool Test(World &world, const GC_Vehicle *v) const;

public:
	GC_Trigger(World &world, float x, float y);
	GC_Trigger(FromFile);
	~GC_Trigger();

	virtual void MapExchange(World &world, MapFile &f);
	virtual void Serialize(World &world, SaveFile &f);

	virtual void TimeStepFixed(World &world, float dt);
	virtual void Draw(bool editorMode) const;
};

// end of file

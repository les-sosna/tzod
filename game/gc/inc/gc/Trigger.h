#pragma once
#include "MovingObject.h"
#include "ObjPtr.h"

#define GC_FLAG_TRIGGER_ENABLED       (GC_FLAG_MO_ << 0)
#define GC_FLAG_TRIGGER_ONLYVISIBLE   (GC_FLAG_MO_ << 1)
#define GC_FLAG_TRIGGER_ONLYHUMAN     (GC_FLAG_MO_ << 2)
#define GC_FLAG_TRIGGER_ACTIVATED     (GC_FLAG_MO_ << 3)
#define GC_FLAG_TRIGGER_              (GC_FLAG_MO_ << 4)

class GC_Vehicle;

class GC_Trigger : public GC_MovingObject
{
	DECLARE_SELF_REGISTRATION(GC_Trigger);
	DECLARE_LIST_MEMBER(override);

public:
	explicit GC_Trigger(vec2d pos);
	explicit GC_Trigger(FromFile);
	~GC_Trigger();

	std::string_view GetOnEnter() const { return _onEnter; }
	std::string_view GetOnLeave() const { return _onLeave; }

	// GC_Object
	void MapExchange(MapFile &f) override;
	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;

private:
	class MyPropertySet : public GC_MovingObject::MyPropertySet
	{
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);

	private:
		typedef GC_MovingObject::MyPropertySet BASE;
		ObjectProperty _propActive;
		ObjectProperty _propTeam;
		ObjectProperty _propRadius;
		ObjectProperty _propRadiusDelta;
		ObjectProperty _propOnlyVisible;
		ObjectProperty _propOnlyHuman;
		ObjectProperty _propOnEnter;
		ObjectProperty _propOnLeave;
	};

	PropertySet* NewPropertySet() override;

	float    _radius;
	float    _radiusDelta;
	int      _team;
	std::string _onEnter;
	std::string _onLeave;

	ObjPtr<GC_Vehicle> _veh;

	bool GetVisible(World &world, const GC_Vehicle *v) const;
	bool Test(World &world, const GC_Vehicle *v) const;
};

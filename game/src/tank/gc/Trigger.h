// Trigger.h

#pragma once


#include "2dSprite.h"

#define GC_FLAG_TRIGGER_          GC_FLAG_2DSPRITE_




class GC_Trigger : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Trigger);

	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propRadius;
		ObjectProperty _propOnEnter;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool applyToObject);
	};

	virtual PropertySet* NewPropertySet();

	float  _radius;

protected:
	virtual GC_Vehicle* FindVehicle() const;


public:
	GC_Trigger(float x, float y);
	GC_Trigger(FromFile);
	~GC_Trigger();

	virtual void mapExchange(MapFile &f);
	virtual void Serialize(SaveFile &f);
	virtual bool IsSaved() { return true; }

	virtual void TimeStepFixed(float dt);
};

// end of file

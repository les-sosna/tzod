// indicators.h

#pragma once

#include "Actor.h"

class GC_Vehicle;

class GC_SpawnPoint : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_SpawnPoint);
    typedef GC_Actor base;

public:
	int   _team;    // 0 - no team

protected:
	class MyPropertySet : public GC_Actor::MyPropertySet
	{
		typedef GC_Actor::MyPropertySet BASE;
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
    DECLARE_LIST_MEMBER();
	GC_SpawnPoint(World &world);
	GC_SpawnPoint(FromFile);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_HideLabel : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_HideLabel);
public:
	GC_HideLabel(World &world);
	GC_HideLabel(FromFile);
};

// end of file

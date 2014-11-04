#pragma once

#include "Actor.h"

class GC_Vehicle;

class GC_SpawnPoint : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_SpawnPoint);
    DECLARE_LIST_MEMBER();
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
	explicit GC_SpawnPoint(vec2d pos);
	explicit GC_SpawnPoint(FromFile);

	// GC_Object
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_HideLabel : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_HideLabel);
public:
	GC_HideLabel(vec2d pos);
	GC_HideLabel(FromFile);
};

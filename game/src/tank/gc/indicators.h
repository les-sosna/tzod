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
    DECLARE_LIST_MEMBER();
	GC_SpawnPoint(World &world);
	GC_SpawnPoint(FromFile);

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_EDITOR; }

	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_HideLabel : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_HideLabel);

public:
	GC_HideLabel(World &world);
	GC_HideLabel(FromFile);

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_EDITOR; }
};

// end of file

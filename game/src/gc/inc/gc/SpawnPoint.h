#pragma once
#include "MovingObject.h"

class GC_Vehicle;

class GC_SpawnPoint : public GC_MovingObject
{
	DECLARE_SELF_REGISTRATION(GC_SpawnPoint);
	DECLARE_LIST_MEMBER(override);

public:
	int _team;    // 0 - no team

public:
	explicit GC_SpawnPoint(vec2d pos);
	explicit GC_SpawnPoint(FromFile);

	// GC_Object
	void Serialize(World &world, SaveFile &f) override;
	void MapExchange(MapFile &f) override;

protected:
	class MyPropertySet : public GC_MovingObject::MyPropertySet
	{
		typedef GC_MovingObject::MyPropertySet BASE;
		ObjectProperty _propTeam;
		ObjectProperty _propDir;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	PropertySet* NewPropertySet() override;
};

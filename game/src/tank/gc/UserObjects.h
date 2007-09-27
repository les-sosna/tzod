// UserObject.h

#pragma once

#include "RigidBody.h"

class GC_UserObject : public GC_RigidBodyStatic
{
	DECLARE_SELF_REGISTRATION(GC_UserObject);

	string_t _textureName;

protected:
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propTexture;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_UserObject(float x, float y);
	GC_UserObject(FromFile);
	virtual ~GC_UserObject();

	virtual unsigned char GetPassability() const { return 1; }
	virtual float GetDefaultHealth() const { return 500; }

	virtual void Serialize(SaveFile &f);
	virtual void OnDestroy();

	virtual void mapExchange(MapFile &f);

};

// end of file

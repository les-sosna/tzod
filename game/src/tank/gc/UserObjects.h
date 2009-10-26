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
		virtual void MyExchange(bool applyToObject);
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

	virtual void MapExchange(MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Decoration : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Decoration);

	string_t _textureName;
	float _frameRate;
	float _time;

protected:
	class MyPropertySet : public GC_2dSprite::MyPropertySet
	{
		typedef GC_2dSprite::MyPropertySet BASE;
		ObjectProperty _propTexture;
		ObjectProperty _propLayer;
		ObjectProperty _propAnimate;
		ObjectProperty _propFrame;
		ObjectProperty _propRotation;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_Decoration(float x, float y);
	GC_Decoration(FromFile);
	virtual ~GC_Decoration();

	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);

	virtual void TimeStepFixed(float dt);
};


// end of file

// UserObject.h

#pragma once

#include "RigidBody.h"

class GC_UserObject : public GC_RigidBodyStatic
{
	DECLARE_SELF_REGISTRATION(GC_UserObject);

	std::string _textureName;

protected:
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propTexture;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(Level &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_UserObject(Level &world, float x, float y);
	GC_UserObject(FromFile);
	virtual ~GC_UserObject();

	virtual unsigned char GetPassability() const { return 1; }
	virtual float GetDefaultHealth() const { return 500; }

	virtual void Serialize(Level &world, SaveFile &f);
	virtual void OnDestroy(Level &world);

	virtual void MapExchange(Level &world, MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Decoration : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Decoration);

	std::string _textureName;
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
		virtual void MyExchange(Level &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_Decoration(Level &world, float x, float y);
	GC_Decoration(FromFile);
	virtual ~GC_Decoration();

	virtual void Serialize(Level &world, SaveFile &f);
	virtual void MapExchange(Level &world, MapFile &f);

	virtual void TimeStepFixed(Level &world, float dt);
};


// end of file

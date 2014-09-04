// UserObject.h

#pragma once

#include "RigidBody.h"

class GC_UserObject : public GC_RigidBodyStatic
{
	DECLARE_SELF_REGISTRATION(GC_UserObject);

	std::string _textureName;
	enumZOrder _zOrder;

protected:
	class MyPropertySet : public GC_RigidBodyStatic::MyPropertySet
	{
		typedef GC_RigidBodyStatic::MyPropertySet BASE;
		ObjectProperty _propTexture;
	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_UserObject(World &world);
	GC_UserObject(FromFile);
	virtual ~GC_UserObject();

	void SetZ(enumZOrder z);
	enumZOrder GetZ() const { return _zOrder; }
	void SetTextureName(std::string name) { _textureName.swap(name); }
	const std::string& GetTextureName() const { return _textureName; }

	virtual unsigned char GetPassability() const { return 1; }
	virtual float GetDefaultHealth() const { return 500; }

	virtual void Serialize(World &world, SaveFile &f);
	virtual void OnDestroy(World &world);

	virtual void MapExchange(World &world, MapFile &f);
};

///////////////////////////////////////////////////////////////////////////////

class GC_Decoration : public GC_2dSprite
{
	DECLARE_SELF_REGISTRATION(GC_Decoration);

	std::string _textureName;
	float _frameRate;
	float _time;
	enumZOrder _zOrder;	

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
		virtual void MyExchange(World &world, bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

public:
	GC_Decoration(World &world);
	GC_Decoration(FromFile);
	virtual ~GC_Decoration();

	void SetZ(enumZOrder z);

	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);

	virtual void TimeStepFixed(World &world, float dt);
};


// end of file

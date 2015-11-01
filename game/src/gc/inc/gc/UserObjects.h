#pragma once
#include "RigidBody.h"
#include "Z.h"

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
	PropertySet* NewPropertySet() override;

public:
	explicit GC_UserObject(vec2d pos);
	explicit GC_UserObject(FromFile);
	virtual ~GC_UserObject();

	void SetZ(enumZOrder z);
	enumZOrder GetZ() const { return _zOrder; }
	void SetTextureName(std::string name) { _textureName.swap(name); }
	const std::string& GetTextureName() const { return _textureName; }

	unsigned char GetPassability() const override { return 1; }
	float GetDefaultHealth() const override { return 500; }

	void Serialize(World &world, SaveFile &f) override;
	void OnDestroy(World &world, const DamageDesc &dd) override;

	void MapExchange(MapFile &f) override;
};

///////////////////////////////////////////////////////////////////////////////

class GC_Decoration : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Decoration);

	std::string _textureName;
	float _frameRate;
	float _time;
	enumZOrder _zOrder;

protected:
	class MyPropertySet : public GC_Actor::MyPropertySet
	{
		typedef GC_Actor::MyPropertySet BASE;
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
	PropertySet* NewPropertySet() override;

public:
	explicit GC_Decoration(vec2d pos);
	explicit GC_Decoration(FromFile);
	virtual ~GC_Decoration();

	void SetZ(enumZOrder z);
	enumZOrder GetZ() const { return _zOrder; }
	void SetTextureName(std::string name) { _textureName.swap(name); }
	const std::string& GetTextureName() const { return _textureName; }

	// GC_Object
	void MapExchange(MapFile &f) override;
	void Serialize(World &world, SaveFile &f) override;
	void TimeStep(World &world, float dt) override;
};


// end of file

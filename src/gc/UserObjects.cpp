#include "TypeReg.h"
#include "inc/gc/UserObjects.h"
#include "inc/gc/Explosion.h"
#include "inc/gc/World.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>

IMPLEMENT_SELF_REGISTRATION(GC_UserObject)
{
	ED_MOVING_OBJECT("user_object", "obj_user_object", 0, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE/2, 0);
	return true;
}

GC_UserObject::GC_UserObject(vec2d pos)
  : GC_RigidBodyStatic(pos)
  , _textureName("turret_platform")
  , _zOrder(Z_WALLS)
{
	SetSize(WORLD_BLOCK_SIZE * 2, WORLD_BLOCK_SIZE * 2);
}

GC_UserObject::GC_UserObject(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_UserObject::~GC_UserObject()
{
}

void GC_UserObject::SetZ(enumZOrder z)
{
	assert(z < Z_COUNT || z == Z_NONE);
	_zOrder = z;
}

void GC_UserObject::Serialize(World &world, SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(world, f);
	f.Serialize(_textureName);
	f.Serialize(_zOrder);
}

void GC_UserObject::OnDestroy(World &world, const DamageDesc &dd)
{
	world.New<GC_ExplosionBig>(GetPos());
	GC_RigidBodyStatic::OnDestroy(world, dd);
}

void GC_UserObject::MapExchange(MapFile &f)
{
	GC_RigidBodyStatic::MapExchange(f);
	MAP_EXCHANGE_STRING(texture, _textureName, "");
}

PropertySet* GC_UserObject::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_UserObject::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTexture( ObjectProperty::TYPE_TEXTURE, "texture" )
{
}

int GC_UserObject::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_UserObject::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTexture;
	}

	assert(false);
	return nullptr;
}

void GC_UserObject::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_UserObject *tmp = static_cast<GC_UserObject *>(GetObject());

	if( applyToObject )
	{
//		world._field.ProcessObject(tmp, false);
		tmp->SetTextureName(std::string(_propTexture.GetStringValue()));
//		tmp->AlignToTexture();
//		world._field.ProcessObject(tmp, true);
	}
	else
	{
		_propTexture.SetStringValue(tmp->GetTextureName());
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Decoration)
{
	ED_MOVING_OBJECT("user_sprite", "obj_user_sprite", 7, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE, WORLD_BLOCK_SIZE/2, 0);
	return true;
}

GC_Decoration::GC_Decoration(vec2d pos)
  : GC_MovingObject(pos)
  , _textureName("turret_platform")
  , _frameRate(0)
  , _time(0)
  , _zOrder(Z_EDITOR)
{
}

GC_Decoration::GC_Decoration(FromFile)
  : GC_MovingObject(FromFile())
{
}

GC_Decoration::~GC_Decoration()
{
}

void GC_Decoration::SetZ(enumZOrder z)
{
	assert(z < Z_COUNT || z == Z_NONE);
	_zOrder = z;
}

void GC_Decoration::Serialize(World &world, SaveFile &f)
{
	GC_MovingObject::Serialize(world, f);
	f.Serialize(_textureName);
	f.Serialize(_frameRate);
	f.Serialize(_time);
	f.Serialize(_zOrder);
}

void GC_Decoration::MapExchange(MapFile &f)
{
	GC_MovingObject::MapExchange(f);

	int z = GetZ();
	int frame = 0;//GetCurrentFrame();
	float rot = GetDirection().Angle();

	MAP_EXCHANGE_STRING(texture, _textureName, "");
	MAP_EXCHANGE_INT(layer, z, 0);
	MAP_EXCHANGE_INT(frame, frame, 0);
	MAP_EXCHANGE_FLOAT(animate, _frameRate, 0);
	MAP_EXCHANGE_FLOAT(rotation, rot, 0);

	if( f.loading() )
	{
//		SetFrame(frame % GetFrameCount());
		SetZ((enumZOrder) z);
		SetDirection(Vec2dDirection(rot));
		if( _frameRate > 0 )
		{
            // TODO: animation
//			SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
		}
		_time = 0;
	}
}

void GC_Decoration::TimeStep(World &world, float dt)
{
	assert(_frameRate > 0);
	_time += dt;
	if( _time * _frameRate > 1 )
	{
//		SetFrame((GetCurrentFrame() + int(_time * _frameRate)) % GetFrameCount());
		_time -= floor(_time * _frameRate) / _frameRate;
	}
}

PropertySet* GC_Decoration::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Decoration::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTexture(ObjectProperty::TYPE_TEXTURE, "texture")
  , _propLayer(ObjectProperty::TYPE_INTEGER, "layer")
  , _propAnimate(ObjectProperty::TYPE_FLOAT, "animate")
  , _propFrame(ObjectProperty::TYPE_INTEGER, "frame")
  , _propRotation(ObjectProperty::TYPE_FLOAT, "rotation")
{
	_propLayer.SetIntRange(0, Z_COUNT-1);
	_propAnimate.SetFloatRange(0, 100);
	_propFrame.SetIntRange(0, 1000);
	_propRotation.SetFloatRange(0, PI2);
}

int GC_Decoration::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 5;
}

ObjectProperty* GC_Decoration::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTexture;
	case 1: return &_propLayer;
	case 2: return &_propAnimate;
	case 3: return &_propFrame;
	case 4: return &_propRotation;
	}

	assert(false);
	return nullptr;
}

void GC_Decoration::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Decoration *tmp = static_cast<GC_Decoration *>(GetObject());

	if( applyToObject )
	{
		tmp->SetTextureName(std::string(_propTexture.GetStringValue()));
		tmp->SetZ((enumZOrder) _propLayer.GetIntValue());
//		tmp->SetFrame(_propFrame.GetIntValue() % tmp->GetFrameCount());
		tmp->SetDirection(Vec2dDirection(_propRotation.GetFloatValue()));
		tmp->_frameRate = _propAnimate.GetFloatValue();

        // TODO: animation
//		tmp->SetEvents(world, tmp->_frameRate > 0 ? GC_FLAG_OBJECT_EVENTS_TS_FIXED : 0);
	}
	else
	{
		_propTexture.SetStringValue(tmp->GetTextureName());
		_propLayer.SetIntValue(tmp->GetZ());
		_propRotation.SetFloatValue(tmp->GetDirection().Angle());
		_propFrame.SetIntValue(0); //tmp->GetCurrentFrame());
		_propAnimate.SetFloatValue(tmp->_frameRate);
	}
}

// end of file

// UserObject.cpp

#include "stdafx.h"

#include "UserObjects.h"
#include "GameClasses.h"

#include "level.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"
#include "Sound.h"

#include "particles.h"
#include "config/Config.h"
#include "functions.h"
#include "video/TextureManager.h"


IMPLEMENT_SELF_REGISTRATION(GC_UserObject)
{
	ED_ACTOR("user_object", "obj_user_object", 0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_UserObject::GC_UserObject(float x, float y)
{
	_textureName = "turret_platform";
	SetZ(Z_WALLS);
	MoveTo(vec2d(x, y));
	SetTexture(_textureName.c_str());
	AlignToTexture();
	g_level->_field.ProcessObject(this, true);
}

GC_UserObject::GC_UserObject(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_UserObject::~GC_UserObject()
{
}

void GC_UserObject::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);
	f.Serialize(_textureName);
}

void GC_UserObject::OnDestroy()
{
	new GC_Boom_Big(GetPos(), NULL);
	GC_RigidBodyStatic::OnDestroy();
}

void GC_UserObject::MapExchange(MapFile &f)
{
	GC_RigidBodyStatic::MapExchange(f);

	MAP_EXCHANGE_STRING(texture, _textureName, "");

	if( f.loading() )
	{
		SetTexture(_textureName.c_str());
	}
}

PropertySet* GC_UserObject::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_UserObject::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTexture( ObjectProperty::TYPE_MULTISTRING, "texture" )
{
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, NULL, false);
	for( size_t i = 1; i < names.size(); ++i )
	{
		const LogicalTexture &lt = g_texman->Get(g_texman->FindSprite(names[i]));
		if( lt.pxFrameWidth <= LOCATION_SIZE / 2 && lt.pxFrameHeight <= LOCATION_SIZE / 2 )
		{
			// only allow using textures which are less than half of cell
			_propTexture.AddItem(names[i]);
		}
	}
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
	return NULL;
}

void GC_UserObject::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_UserObject *tmp = static_cast<GC_UserObject *>(GetObject());

	if( applyToObject )
	{
		g_level->_field.ProcessObject(tmp, false);
		tmp->_textureName = _propTexture.GetListValue(_propTexture.GetCurrentIndex());
		tmp->SetTexture(tmp->_textureName.c_str());
		tmp->AlignToTexture();
		g_level->_field.ProcessObject(tmp, true);
	}
	else
	{
		for( size_t i = 0; i < _propTexture.GetListSize(); ++i )
		{
			if( tmp->_textureName == _propTexture.GetListValue(i) )
			{
				_propTexture.SetCurrentIndex(i);
				break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Decoration)
{
	ED_ACTOR("user_sprite", "obj_user_sprite", 7, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_Decoration::GC_Decoration(float x, float y)
  : _textureName("turret_platform")
  , _frameRate(0)
  , _time(0)
{
	SetZ(Z_EDITOR);
	MoveTo(vec2d(x, y));
	SetTexture(_textureName.c_str());
}

GC_Decoration::GC_Decoration(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Decoration::~GC_Decoration()
{
}

void GC_Decoration::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_textureName);
	f.Serialize(_frameRate);
	f.Serialize(_time);
}

void GC_Decoration::MapExchange(MapFile &f)
{
	GC_2dSprite::MapExchange(f);

	int z = GetZ();
	int frame = GetCurrentFrame();
	float rot = GetDirection().Angle();

	MAP_EXCHANGE_STRING(texture, _textureName, "");
	MAP_EXCHANGE_INT(layer, z, 0);
	MAP_EXCHANGE_INT(frame, frame, 0);
	MAP_EXCHANGE_FLOAT(animate, _frameRate, 0);
	MAP_EXCHANGE_FLOAT(rotation, rot, 0);

	if( f.loading() )
	{
		SetTexture(_textureName.c_str());
		SetFrame(frame % GetFrameCount());
		SetZ((enumZOrder) z);
		SetDirection(vec2d(rot));
		if( _frameRate > 0 )
		{
			SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
		}
		_time = 0;
	}
}

void GC_Decoration::TimeStepFixed(float dt)
{
	assert(_frameRate > 0);
	_time += dt;
	if( _time * _frameRate > 1 )
	{
		SetFrame((GetCurrentFrame() + int(_time * _frameRate)) % GetFrameCount());
		_time -= floor(_time * _frameRate) / _frameRate;
	}
}

PropertySet* GC_Decoration::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Decoration::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTexture(ObjectProperty::TYPE_MULTISTRING, "texture")
  , _propLayer(ObjectProperty::TYPE_INTEGER, "layer")
  , _propAnimate(ObjectProperty::TYPE_FLOAT, "animate")
  , _propFrame(ObjectProperty::TYPE_INTEGER, "frame")
  , _propRotation(ObjectProperty::TYPE_FLOAT, "rotation")
{
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, NULL, false);
	for( size_t i = 1; i < names.size(); ++i )
	{
		const LogicalTexture &lt = g_texman->Get(g_texman->FindSprite(names[i]));
		//if( lt.pxFrameWidth <= LOCATION_SIZE / 2 && lt.pxFrameHeight <= LOCATION_SIZE / 2 )
		//{
			// only allow using textures which are less than half of cell
			_propTexture.AddItem(names[i]);
		//}
	}
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
	return NULL;
}

void GC_Decoration::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Decoration *tmp = static_cast<GC_Decoration *>(GetObject());

	if( applyToObject )
	{
		tmp->_textureName = _propTexture.GetListValue(_propTexture.GetCurrentIndex());
		tmp->SetTexture(tmp->_textureName.c_str());
		tmp->SetZ((enumZOrder) _propLayer.GetIntValue());
		tmp->SetFrame(_propFrame.GetIntValue() % tmp->GetFrameCount());
		tmp->SetDirection(vec2d(_propRotation.GetFloatValue()));
		tmp->_frameRate = _propAnimate.GetFloatValue();
		tmp->SetEvents(tmp->_frameRate > 0 ? GC_FLAG_OBJECT_EVENTS_TS_FIXED : 0);
	}
	else
	{
		for( size_t i = 0; i < _propTexture.GetListSize(); ++i )
		{
			if( tmp->_textureName == _propTexture.GetListValue(i) )
			{
				_propTexture.SetCurrentIndex(i);
				break;
			}
		}
		_propLayer.SetIntValue(tmp->GetZ());
		_propRotation.SetFloatValue(tmp->GetDirection().Angle());
		_propFrame.SetIntValue(tmp->GetCurrentFrame());
		_propAnimate.SetFloatValue(tmp->_frameRate);
	}
}

// end of file

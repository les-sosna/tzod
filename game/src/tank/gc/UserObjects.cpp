// UserObject.cpp

#include "stdafx.h"

#include "UserObject.h"
#include "GameClasses.h"

#include "level.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "video/TextureManager.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_UserObject)
{
	ED_ACTOR("user_object", "Особый объект", 0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
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

GC_UserObject::GC_UserObject(FromFile) : GC_RigidBodyStatic(FromFile())
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
	new GC_Boom_Big( GetPos(), NULL);
	GC_RigidBodyStatic::OnDestroy();
}

void GC_UserObject::mapExchange(MapFile &f)
{
	GC_RigidBodyStatic::mapExchange(f);

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
		_propTexture.AddItem(names[i]);
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

	_ASSERT(FALSE);
	return NULL;
}

void GC_UserObject::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

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

// end of file

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
	ED_ACTOR("user_object", "Особый объект", 1, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, 0);
	return true;
}

GC_UserObject::GC_UserObject(float x, float y)
{
	SetZ(Z_WALLS);
	MoveTo(vec2d(x, y));
	SetTexture("turret_platform");
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
}

void GC_UserObject::mapExchange(MapFile &f)
{
	GC_RigidBodyStatic::mapExchange(f);

	MAP_EXCHANGE_STRING(texture, _textureName, "");

}

SafePtr<PropertySet> GC_UserObject::GetProperties()
{
	return new MyPropertySet(this);
}

GC_UserObject::MyPropertySet::MyPropertySet(GC_Object *object)
: BASE(object)
, _propTexture( ObjectProperty::TYPE_MULTISTRING, "Texture" )
{
	std::vector<string_t> names;
	g_texman->GetTextureNames(names, NULL, false);
	for( size_t i = 1; i < names.size(); ++i )
	{
		_propTexture.AddItem(names[i]);
	}

	Exchange(false);
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
		tmp->_textureName = _propTexture.GetSetValue(_propTexture.GetCurrentIndex());
		tmp->SetTexture(tmp->_textureName.c_str());
		tmp->AlignToTexture();
		g_level->_field.ProcessObject(tmp, true);
	}
	else
	{
		for( size_t i = 0; i < _propTexture.GetSetSize(); ++i )
		{
			if( tmp->_textureName == _propTexture.GetSetValue(i) )
			{
				_propTexture.SetCurrentIndex(i);
				break;
			}
		}
	}
}

// end of file

#include "Object.h"
#include "World.h"
#include "WorldEvents.h"

#include "MapFile.h"
#include "SaveFile.h"

#include "core/Debug.h"


PropertySet::PropertySet(GC_Object *object)
  : _object(*object)
  , _propName(ObjectProperty::TYPE_STRING, "name")
{
}

int PropertySet::GetCount() const
{
	return 1;  // name
}

ObjectProperty* PropertySet::GetProperty(int index)
{
	assert(index < GetCount());
	return &_propName;
}

void PropertySet::MyExchange(World &world, bool applyToObject)
{
	if( applyToObject )
	{
		const char *name = _propName.GetStringValue().c_str();
		GC_Object* found = world.FindObject(name);
		if( found && GetObject() != found )
		{
			GetConsole().Format(1) << "object with name \"" << name << "\" already exists";
		}
		else
		{
			GetObject()->SetName(world, name);
		}
	}
	else
	{
		const char *name = GetObject()->GetName(world);
		_propName.SetStringValue(name ? name : "");
	}
}

void PropertySet::Exchange(World &world, bool applyToObject)
{
	MyExchange(world, applyToObject);
}

///////////////////////////////////////////////////////////////////////////////

// custom IMPLEMENT_1LIST_MEMBER for base class
ObjectList::id_type GC_Object::Register(World &world)
{
	assert(ObjectList::id_type() == _posLIST_objects);
    _posLIST_objects = world.GetList(LIST_objects).insert(this);
    return _posLIST_objects;
}
void GC_Object::Unregister(World &world, ObjectList::id_type pos)
{
    world.GetList(LIST_objects).erase(pos);
}


GC_Object::GC_Object()
  : _flags(0)
{
}

GC_Object::~GC_Object()
{
}

void GC_Object::Kill(World &world)
{
	world.OnKill(*this);
	SetName(world, NULL);
    Unregister(world, _posLIST_objects);
	delete this;
}

void GC_Object::Serialize(World &world, SaveFile &f)
{
	f.Serialize(_flags);

	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		if( f.loading() )
		{
			std::string name;
			f.Serialize(name);

			assert( 0 == world._objectToStringMap.count(this) );
			assert( 0 == world._nameToObjectMap.count(name) );
			world._objectToStringMap[this] = name;
			world._nameToObjectMap[name] = this;
		}
		else
		{
			std::string name = GetName(world);
			f.Serialize(name);
		}
	}
}

const char* GC_Object::GetName(World &world) const
{
	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		assert( world._objectToStringMap.count(this) );
		return world._objectToStringMap[this].c_str();
	}
	return NULL;
}

void GC_Object::SetName(World &world, const char *name)
{
	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		//
		// remove old name
		//

		assert(world._objectToStringMap.count(this));
		const std::string &oldName = world._objectToStringMap[this];
		assert(world._nameToObjectMap.count(oldName));
		world._nameToObjectMap.erase(oldName);
		world._objectToStringMap.erase(this); // this invalidates oldName ref
		SetFlags(GC_FLAG_OBJECT_NAMED, false);
	}

	if( name && *name )
	{
		//
		// set new name
		//

		assert( 0 == world._objectToStringMap.count(this) );
		assert( 0 == world._nameToObjectMap.count(name) );

		world._objectToStringMap[this] = name;
		world._nameToObjectMap[name] = this;

		SetFlags(GC_FLAG_OBJECT_NAMED, true);
	}
}

void GC_Object::Init(World &world)
{
}

void GC_Object::Resume(World &world)
{
}

void GC_Object::TimeStep(World &world, float dt)
{
}

std::shared_ptr<PropertySet> GC_Object::GetProperties(World &world)
{
	std::shared_ptr<PropertySet> ps(NewPropertySet());
	ps->Exchange(world, false); // fill property set with data from object
	return std::move(ps);
}

PropertySet* GC_Object::NewPropertySet()
{
	return new MyPropertySet(this);
}

void GC_Object::MapExchange(MapFile &f)
{
}

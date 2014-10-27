// Object.cpp

#include "Object.h"

#include "World.h"
#include "WorldEvents.h"
#include "MapFile.h"
#include "SaveFile.h"

#include "config/Config.h"
#include "core/Debug.h"

///////////////////////////////////////////////////////////////////////////////
// PropertySet class implementation

PropertySet::PropertySet(GC_Object *object)
  : _object(object),
  _propName(ObjectProperty::TYPE_STRING, "name")
{
}

void PropertySet::LoadFromConfig()
{
	ConfVarTable *op = g_conf.ed_objproperties.GetTable(RTTypes::Inst().GetTypeName(_object->GetType()));
	for( int i = 0; i < GetCount(); ++i )
	{
		ObjectProperty *prop = GetProperty(i);
		switch( prop->GetType() )
		{
		case ObjectProperty::TYPE_INTEGER:
            prop->SetIntValue(std::min(prop->GetIntMax(),
                                       std::max(prop->GetIntMin(),
                                                op->GetNum(prop->GetName(), prop->GetIntValue())->GetInt())));
			break;
		case ObjectProperty::TYPE_FLOAT:
            prop->SetFloatValue(std::min(prop->GetFloatMax(),
                                         std::max(prop->GetFloatMin(),
                                                  op->GetNum(prop->GetName(), prop->GetFloatValue())->GetFloat())));
			break;
		case ObjectProperty::TYPE_STRING:
		case ObjectProperty::TYPE_SKIN:
		case ObjectProperty::TYPE_TEXTURE:
			prop->SetStringValue(op->GetStr(prop->GetName(), prop->GetStringValue().c_str())->Get());
			break;
		case ObjectProperty::TYPE_MULTISTRING:
            prop->SetCurrentIndex(std::min((int) prop->GetListSize() - 1,
                                           std::max(0, op->GetNum(prop->GetName(), (int) prop->GetCurrentIndex())->GetInt())));
			break;
		default:
			assert(false);
		} // end of switch( prop->GetType() )
	}
}

void PropertySet::SaveToConfig()
{
	ConfVarTable *op = g_conf.ed_objproperties.GetTable(RTTypes::Inst().GetTypeName(_object->GetType()));
	for( int i = 0; i < GetCount(); ++i )
	{
		ObjectProperty *prop = GetProperty(i);
		switch( prop->GetType() )
		{
		case ObjectProperty::TYPE_INTEGER:
			op->SetNum(prop->GetName(), prop->GetIntValue());
			break;
		case ObjectProperty::TYPE_FLOAT:
			op->SetNum(prop->GetName(), prop->GetFloatValue());
			break;
		case ObjectProperty::TYPE_STRING:
		case ObjectProperty::TYPE_SKIN:
		case ObjectProperty::TYPE_TEXTURE:
			op->SetStr(prop->GetName(), prop->GetStringValue());
			break;
		case ObjectProperty::TYPE_MULTISTRING:
			op->SetNum(prop->GetName(), (int) prop->GetCurrentIndex());
			break;
		default:
			assert(false);
		} // end of switch( prop->GetType() )
	}}

int PropertySet::GetCount() const
{
	return 1;  // name
}

GC_Object* PropertySet::GetObject() const
{
	return _object;
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
// GC_Object class implementation

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
  , _firstNotify(NULL)
  , _notifyProtectCount(0)
{
}

GC_Object::~GC_Object()
{
	assert(0 == _notifyProtectCount);
	while( _firstNotify )
	{
		Notify *n = _firstNotify;
		_firstNotify = n->next;
		delete n;
	}
}

void GC_Object::Kill(World &world)
{
	world.OnKill(*this);
	PulseNotify(world, NOTIFY_OBJECT_KILL);
	SetName(world, NULL);
    Unregister(world, _posLIST_objects);
	delete this;
}

IMPLEMENT_POOLED_ALLOCATION(GC_Object::Notify);

void GC_Object::Notify::Serialize(World &world, SaveFile &f)
{
	f.Serialize(type);
	f.Serialize(subscriber);

	// we are not allowed to serialize raw pointers so we use a small hack :)
	f.Serialize(reinterpret_cast<size_t&>(handler));
}

void GC_Object::Serialize(World &world, SaveFile &f)
{
	assert(0 == _notifyProtectCount);

	f.Serialize(_flags);


	//
	// name
	//

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


	//
	// notifications
	//

	size_t count = 0;
	if( const Notify *n = _firstNotify )
		do { count += !n->IsRemoved(); } while( (n = n->next) );
	f.Serialize(count);
	if( f.loading() )
	{
		assert(NULL == _firstNotify);
		for( size_t i = 0; i < count; i++ )
		{
			_firstNotify = new Notify(_firstNotify);
			_firstNotify->Serialize(world, f);
		}
	}
	else
	{
		for( Notify *n = _firstNotify; n; n = n->next )
		{
			if( !n->IsRemoved() )
				n->Serialize(world, f);
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

void GC_Object::Subscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler)
{
	assert(subscriber);
	assert(handler);
	//--------------------------------------------------
	_firstNotify = new Notify(_firstNotify);
	_firstNotify->type        = type;
	_firstNotify->subscriber  = subscriber;
	_firstNotify->handler     = handler;
}

void GC_Object::Unsubscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler)
{
	assert(subscriber);
	for( Notify *prev = NULL, *n = _firstNotify; n; n = n->next )
	{
		if( type == n->type && subscriber == n->subscriber && handler == n->handler )
		{
			if( _notifyProtectCount )
			{
				n->subscriber = NULL;
			}
			else
			{
				(prev ? prev->next : _firstNotify) = n->next;
				delete n;
			}
			return;
		}
		prev = n;
	}
	assert(!"subscription not found");
}

void GC_Object::PulseNotify(World &world, NotifyType type, void *param)
{
	++_notifyProtectCount;
	for( Notify *n = _firstNotify; n; n = n->next )
	{
		if( type == n->type && !n->IsRemoved() )
		{
			((n->subscriber)->*n->handler)(world, this, param);
		}
	}
	--_notifyProtectCount;
	if( 0 == _notifyProtectCount )
	{
		for( Notify *prev = NULL, *n = _firstNotify; n; )
		{
			if( n->IsRemoved() )
			{
				Notify *&pp = prev ? prev->next : _firstNotify;
				pp = n->next;
				delete n;
				n = pp;
			}
			else
			{
				prev = n;
				n = n->next;
			}
		}
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

///////////////////////////////////////////////////////////////////////////////
// end of file

// Object.cpp
///////////////////////////////////////////////////////////////////////////////

#include "Object.h"

#include "GlobalListHelper.inl"
#include "Level.h"
#include "MapFile.h"
#include "SaveFile.h"

#include "config/Config.h"
#include "core/debug.h"

///////////////////////////////////////////////////////////////////////////////
// PropertySet class implementation

PropertySet::PropertySet(GC_Object *object)
  : _object(object),
  _propName(ObjectProperty::TYPE_STRING, "name")
{
}

const char* PropertySet::GetTypeName() const
{
	return RTTypes::Inst().GetTypeName(_object->GetType());
}

void PropertySet::LoadFromConfig()
{
	ConfVarTable *op = g_conf.ed_objproperties.GetTable(GetTypeName());
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
	ConfVarTable *op = g_conf.ed_objproperties.GetTable(GetTypeName());
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

void PropertySet::MyExchange(Level &world, bool applyToObject)
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

void PropertySet::Exchange(Level &world, bool applyToObject)
{
	MyExchange(world, applyToObject);
}

///////////////////////////////////////////////////////////////////////////////
// GC_Object class implementation

GC_Object::GC_Object()
  : _memberOf(this)
  , _flags(0)
  , _firstNotify(NULL)
  , _notifyProtectCount(0)
{
}

GC_Object::GC_Object(FromFile)
  : _memberOf(this)
  , _flags(0) // to clear GC_FLAG_OBJECT_KILLED & GC_FLAG_OBJECT_NAMED for proper handling of bad save files
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
//	assert(world._garbage.erase(this) == 1);
}

void GC_Object::Kill(Level &world)
{
//	assert(world._garbage.insert(this).second);

	PulseNotify(world, NOTIFY_OBJECT_KILL);
	SetEvents(world, 0);
	SetName(world, NULL);
	delete this;
}

//IMPLEMENT_POOLED_ALLOCATION(GC_Object::Notify);

void GC_Object::Notify::Serialize(Level &world, SaveFile &f)
{
	f.Serialize(type);
	f.Serialize(subscriber);

	// we are not allowed to serialize raw pointers so we use a small hack :)
	f.Serialize(reinterpret_cast<size_t&>(handler));
}

void GC_Object::Serialize(Level &world, SaveFile &f)
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

			assert( 0 == world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)].count(this) );
			assert( 0 == world._nameToObjectMap.count(name) );
			world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)][this] = name;
			world._nameToObjectMap[name] = this;
		}
		else
		{
			std::string name = GetName(world);
			f.Serialize(name);
		}
	}


	//
	// events
	//

	if( f.loading() )
	{
		unsigned int tmp = _flags & GC_FLAG_OBJECT_EVENTS_TS_FIXED;
		SetFlags(GC_FLAG_OBJECT_EVENTS_TS_FIXED, false);
		SetEvents(world, tmp);
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

void GC_Object::SetEvents(Level &world, unsigned int dwEvents)
{
	// remove from the TIMESTEP_FIXED list
	if( 0 == (GC_FLAG_OBJECT_EVENTS_TS_FIXED & dwEvents) &&
		0 != (GC_FLAG_OBJECT_EVENTS_TS_FIXED & _flags) )
	{
        world.ts_fixed.safe_erase(_itPosFixed);
	}
	// add to the TIMESTEP_FIXED list
	else if( 0 != (GC_FLAG_OBJECT_EVENTS_TS_FIXED & dwEvents) &&
			 0 == (GC_FLAG_OBJECT_EVENTS_TS_FIXED & _flags) )
	{
		world.ts_fixed.push_front(this);
		_itPosFixed = world.ts_fixed.begin();
	}

	//-------------------------
	SetFlags(GC_FLAG_OBJECT_EVENTS_TS_FIXED, false);
	SetFlags(dwEvents, true);
}

const char* GC_Object::GetName(Level &world) const
{
	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		assert( world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)].count(this) );
		return world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)][this].c_str();
	}
	return NULL;
}

void GC_Object::SetName(Level &world, const char *name)
{
	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		//
		// remove old name
		//

		assert(world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)].count(this));
		const std::string &oldName = world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)][this];
		assert(world._nameToObjectMap.count(oldName));
		world._nameToObjectMap.erase(oldName);
		world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)].erase(this); // this invalidates oldName ref
		SetFlags(GC_FLAG_OBJECT_NAMED, false);
	}

	if( name && *name )
	{
		//
		// set new name
		//

		assert( 0 == world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)].count(this) );
		assert( 0 == world._nameToObjectMap.count(name) );

		world._objectToStringMaps[FastLog2(GC_FLAG_OBJECT_NAMED)][this] = name;
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

void GC_Object::PulseNotify(Level &world, NotifyType type, void *param)
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

void GC_Object::TimeStepFixed(Level &world, float dt)
{
}

void GC_Object::TimeStepFloat(Level &world, float dt)
{
}

void GC_Object::EditorAction(Level &world)
{
}

SafePtr<PropertySet> GC_Object::GetProperties(Level &world)
{
	SafePtr<PropertySet> ps(NewPropertySet());
	ps->Exchange(world, false); // fill property set with data from object
	return ps;
}

PropertySet* GC_Object::NewPropertySet()
{
	return new MyPropertySet(this);
}

void GC_Object::MapExchange(Level &world, MapFile &f)
{
	std::string tmp_name;
	const char *name = GetName(world);
	tmp_name = name ? name : "";
	MAP_EXCHANGE_STRING(name, tmp_name, "");

	if( f.loading() )
	{
		SetName(world, tmp_name.c_str());
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

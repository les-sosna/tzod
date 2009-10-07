// Object.cpp
///////////////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Object.h"
#include "level.h"

#include "config/Config.h"

#include "core/debug.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"


///////////////////////////////////////////////////////////////////////////////
// ObjectProperty class implementation

ObjectProperty::ObjectProperty(PropertyType type, const string_t &name)
  : _type(type)
  , _name(name)
  , _value_index(0)
  , _int_min(0)
  , _int_max(0)
{
}

ObjectProperty::PropertyType ObjectProperty::GetType(void) const
{
	return _type;
}

const string_t& ObjectProperty::GetName(void) const
{
	return _name;
}

int ObjectProperty::GetIntValue(void) const
{
	assert(TYPE_INTEGER == _type);
	return _int_value;
}

int ObjectProperty::GetIntMin(void) const
{
	assert(TYPE_INTEGER == _type);
	return _int_min;
}

int ObjectProperty::GetIntMax(void) const
{
	assert(TYPE_INTEGER == _type);
	return _int_max;
}

void ObjectProperty::SetIntValue(int value)
{
	assert(TYPE_INTEGER == _type);
	assert(value >= GetIntMin());
	assert(value <= GetIntMax());
	_int_value = value;
}

void ObjectProperty::SetIntRange(int min, int max)
{
	assert(TYPE_INTEGER == _type);
	_int_min = min;
	_int_max = max;
}

float ObjectProperty::GetFloatValue(void) const
{
	assert(TYPE_FLOAT == _type);
	return _float_value;
}

float ObjectProperty::GetFloatMin(void) const
{
	assert(TYPE_FLOAT == _type);
	return _float_min;
}

float ObjectProperty::GetFloatMax(void) const
{
	assert(TYPE_FLOAT == _type);
	return _float_max;
}

void ObjectProperty::SetFloatValue(float value)
{
	assert(TYPE_FLOAT == _type);
	assert(value >= GetFloatMin());
	assert(value <= GetFloatMax());
	_float_value = value;
}

void ObjectProperty::SetFloatRange(float min, float max)
{
	assert(TYPE_FLOAT == _type);
	_float_min = min;
	_float_max = max;
}

void ObjectProperty::SetStringValue(const string_t &str)
{
	assert(TYPE_STRING == _type);
	_str_value = str;
}

const string_t& ObjectProperty::GetStringValue(void) const
{
	assert( TYPE_STRING == _type );
	return _str_value;
}

void ObjectProperty::AddItem(const string_t &str)
{
	assert(TYPE_MULTISTRING == _type);
	_value_set.push_back(str);
}

const string_t& ObjectProperty::GetListValue(size_t index) const
{
	assert(TYPE_MULTISTRING == _type);
	assert(index < _value_set.size());
	return _value_set[index];
}

size_t ObjectProperty::GetCurrentIndex(void) const
{
	assert(TYPE_MULTISTRING == _type);
	return _value_index;
}

void ObjectProperty::SetCurrentIndex(size_t index)
{
	assert(TYPE_MULTISTRING == _type);
	assert(index < _value_set.size());
	_value_index = index;
}

size_t ObjectProperty::GetListSize(void) const
{
	return _value_set.size();
}

///////////////////////////////////////////////////////////////////////////////
// PropertySet class implementation

PropertySet::PropertySet(GC_Object *object)
  : _object(object),
  _propName(ObjectProperty::TYPE_STRING, "name")
{
}

const char* PropertySet::GetTypeName() const
{
	return Level::GetTypeName(_object->GetType());
}

void PropertySet::LoadFromConfig()
{
	ConfVarTable *op = g_conf->ed_objproperties->GetTable(GetTypeName());
	for( int i = 0; i < GetCount(); ++i )
	{
		ObjectProperty *prop = GetProperty(i);
		switch( prop->GetType() )
		{
		case ObjectProperty::TYPE_INTEGER:
			prop->SetIntValue(
				__min(prop->GetIntMax(), __max(prop->GetIntMin(),
					op->GetNum(prop->GetName(), prop->GetIntValue())->GetInt()
			)));
			break;
		case ObjectProperty::TYPE_FLOAT:
			prop->SetFloatValue(
				__min(prop->GetFloatMax(), __max(prop->GetFloatMin(),
					op->GetNum(prop->GetName(), prop->GetFloatValue())->GetFloat()
			)));
			break;
		case ObjectProperty::TYPE_STRING:
			prop->SetStringValue(op->GetStr(prop->GetName(), prop->GetStringValue().c_str())->Get());
			break;
		case ObjectProperty::TYPE_MULTISTRING:
			prop->SetCurrentIndex(
				__min((int) prop->GetListSize() - 1, __max(0,
					op->GetNum(prop->GetName(), (int) prop->GetCurrentIndex())->GetInt()
			)));
			break;
		default:
			assert(FALSE);
		} // end of switch( prop->GetType() )
	}
}

void PropertySet::SaveToConfig()
{
	ConfVarTable *op = g_conf->ed_objproperties->GetTable(GetTypeName());
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
			assert(FALSE);
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

void PropertySet::MyExchange(bool applyToObject)
{
	if( applyToObject )
	{
		const char *name = _propName.GetStringValue().c_str();
		GC_Object* found = g_level->FindObject(name);
		if( found && GetObject() != found )
		{
			GetConsole().Format(1) << "object with name \"" << name << "\" already exists";
		}
		else
		{
			GetObject()->SetName(name);
		}
	}
	else
	{
		const char *name = GetObject()->GetName();
		_propName.SetStringValue(name ? name : "");
	}
}

void PropertySet::Exchange(bool applyToObject)
{
	MyExchange(applyToObject);
	if( eventExchange )
		INVOKE(eventExchange)(applyToObject);
}

///////////////////////////////////////////////////////////////////////////////
// GC_Object class implementation

GC_Object::GC_Object()
  : _memberOf(this)
{
	_refCount             = 1;
	_notifyProtectCount   = 0;
	_flags                = 0;
}

GC_Object::GC_Object(FromFile)
  : _memberOf(this)
{
	_notifyProtectCount = 0;
}

GC_Object::~GC_Object()
{
	assert(0 == _refCount);
	assert(0 == _notifyProtectCount);
	assert(IsKilled());
	SetName(NULL);
}

void GC_Object::Kill()
{
	if( IsKilled() ) return;
	SetFlags(GC_FLAG_OBJECT_KILLED, true);

	SetName(NULL);

	// отписка от событий движка
	SetEvents(0);

	PulseNotify(NOTIFY_OBJECT_KILL);
	Release();
}

void GC_Object::Notify::Serialize(SaveFile &f)
{
	f.Serialize(type);
	f.Serialize(once);
	f.Serialize(hasGuard);
	f.Serialize(subscriber);

	// we are not allowed to serialize raw pointers so we use a small hack :)
	f.Serialize(reinterpret_cast<DWORD_PTR&>(handler));
}

void GC_Object::Serialize(SaveFile &f)
{
	f.Serialize(_flags);
	f.Serialize(_refCount);

	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		if( f.loading() )
		{
			string_t name;
			f.Serialize(name);

			assert( 0 == g_level->_objectToNameMap.count(this) );
			assert( 0 == g_level->_nameToObjectMap.count(name) );
			g_level->_objectToNameMap[this] = name;
			g_level->_nameToObjectMap[name] = this;
		}
		else
		{
			string_t name = GetName();
			f.Serialize(name);
		}
	}

	if( f.loading() )
	{
		// events
		DWORD tmp = _flags & GC_FLAG_OBJECT_EVENTS_ALL;
		SetFlags(GC_FLAG_OBJECT_EVENTS_ALL, false);
		SetEvents(tmp);
	}

	// notify list
	unsigned short count = _notifyList.size();
	f.Serialize(count);
	if( f.loading() )
	{
		assert(_notifyList.empty());
		for( int i = 0; i < count; i++ )
		{
			_notifyList.push_back(Notify());
			_notifyList.back().Serialize(f);
		}
	}
	else
	{
		std::list<Notify>::iterator it = _notifyList.begin();
		for( ; it != _notifyList.end(); ++it )
			it->Serialize(f);
	}
}

GC_Object* GC_Object::CreateFromFile(SaveFile &file)
{
	assert(file.loading());

	ObjectType type;
	file.Serialize(type);

	__FromFileMap::const_iterator it = __GetFromFileMap().find(type);
	if( __GetFromFileMap().end() == it )
	{
		TRACE("ERROR: unknown object type - %u", type);
		throw std::runtime_error("Load error: unknown object type");
	}

	GC_Object *object = it->second();

	size_t id;
	file.Serialize(id);
	file.RegPointer(object, id);

	object->Serialize(file);
	return object;
}

int GC_Object::AddRef()
{
	return ++_refCount;
}

int GC_Object::Release()
{
	assert(_refCount > 0);
	if( 0 == (--_refCount) )
	{
		assert(IsKilled());
		delete this;
		return 0;
	}
	return _refCount;
}

void GC_Object::SetEvents(DWORD dwEvents)
{
	// удаление из TIMESTEP_FIXED
	if( 0 == (GC_FLAG_OBJECT_EVENTS_TS_FIXED & dwEvents) &&
		0 != (GC_FLAG_OBJECT_EVENTS_TS_FIXED & _flags) )
	{
        g_level->ts_fixed.safe_erase(_itPosFixed);
	}
	// добавление в TIMESTEP_FIXED
	else if( 0 != (GC_FLAG_OBJECT_EVENTS_TS_FIXED & dwEvents) &&
			 0 == (GC_FLAG_OBJECT_EVENTS_TS_FIXED & _flags) )
	{
		assert(!IsKilled());
		g_level->ts_fixed.push_front(this);
		_itPosFixed = g_level->ts_fixed.begin();
	}

	// удаление из TIMESTEP_FLOATING
	if( 0 != (GC_FLAG_OBJECT_EVENTS_TS_FLOATING & _flags) &&
		0 == (GC_FLAG_OBJECT_EVENTS_TS_FLOATING & dwEvents) )
	{
		g_level->ts_floating.safe_erase(_itPosFloating);
	}
	// добавление в TIMESTEP_FLOATING
	else if( 0 == (GC_FLAG_OBJECT_EVENTS_TS_FLOATING & _flags) &&
			 0 != (GC_FLAG_OBJECT_EVENTS_TS_FLOATING & dwEvents) )
	{
		assert(!IsKilled());
		g_level->ts_floating.push_front(this);
		_itPosFloating = g_level->ts_floating.begin();
	}

	//-------------------------
	SetFlags(GC_FLAG_OBJECT_EVENTS_ALL, false);
	SetFlags(dwEvents, true);
}

const char* GC_Object::GetName() const
{
	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		assert( g_level->_objectToNameMap.count(this) );
		return g_level->_objectToNameMap[this].c_str();
	}
	return NULL;
}

void GC_Object::SetName(const char *name)
{
	if( CheckFlags(GC_FLAG_OBJECT_NAMED) )
	{
		//
		// remove old name
		//

		assert( g_level->_objectToNameMap.count(this) );
		const char *oldName = g_level->_objectToNameMap[this].c_str();
		assert( g_level->_nameToObjectMap.count(oldName) );
		g_level->_nameToObjectMap.erase(oldName);
		g_level->_objectToNameMap.erase(this); // this invalidates *oldName pointer

		SetFlags(GC_FLAG_OBJECT_NAMED, false);
	}

	if( name && *name )
	{
		//
		// set new name
		//

		assert( 0 == g_level->_objectToNameMap.count(this) );
		assert( 0 == g_level->_nameToObjectMap.count(name) );

		g_level->_objectToNameMap[this] = name;
		g_level->_nameToObjectMap[name] = this;

		SetFlags(GC_FLAG_OBJECT_NAMED, true);
	}
}

void GC_Object::Subscribe(NotyfyType type, GC_Object *subscriber,
                          NOTIFYPROC handler, bool once, bool guard)
{
	assert(!IsKilled());
	assert(subscriber && !subscriber->IsKilled());
	assert(handler);
	//--------------------------------------------------
	Notify notify;
	notify.type         = type;
	notify.subscriber   = WrapRawPtr(subscriber);
	notify.handler      = handler;
	notify.once         = once;
	notify.hasGuard     = guard;
	_notifyList.push_back(notify);
	//--------------------------------------------------
	if( guard ) // защита на случай если subscriber умрет раньше, чем this
	{
		notify.type        = NOTIFY_OBJECT_KILL;
		notify.subscriber  = WrapRawPtr(this);
		notify.handler     = &GC_Object::OnKillSubscriber;
		notify.once        = true;
		notify.hasGuard    = false;
		subscriber->_notifyList.push_back(notify);
	}
}

void GC_Object::Unguard(GC_Object *subscriber)
{
	std::list<Notify>::iterator it = _notifyList.begin();
	while( it != _notifyList.end() )
	{
		if( &GC_Object::OnKillSubscriber != it->handler || subscriber != it->subscriber )
		{
			++it;
			continue;
		}
		std::list<Notify>::iterator tmp = it++;
		assert(!tmp->hasGuard);
		assert(NOTIFY_OBJECT_KILL == tmp->type);
		if( _notifyProtectCount )
			tmp->removed = true;
		else
			_notifyList.erase(tmp);
		return;
	}
	assert(FALSE); // guard has not been found
}

void GC_Object::Unsubscribe(GC_Object *subscriber)
{
	std::list<Notify>::iterator it = _notifyList.begin();
	while( it != _notifyList.end() )
	{
		if( subscriber != it->subscriber )
		{
			++it;
			continue;
		}
		std::list<Notify>::iterator tmp = it++;
		if( tmp->hasGuard )
		{
			// find self in the subscriber's list and remove OnKillSubscriber
			tmp->subscriber->Unguard(this);
			tmp->hasGuard = false;
		}
		if( _notifyProtectCount )
			tmp->removed = true;
		else
			_notifyList.erase(tmp);
	}
}

bool GC_Object::IsSubscriber(const GC_Object *object) const
{
	std::list<Notify>::const_iterator it = _notifyList.begin();
	for( ; it != _notifyList.end(); ++it )
	{
		if( it->removed ) continue;
		if( object == it->subscriber ) return true;
	}
	return false;
}

GC_Object* GC_Object::GetSubscriber(ObjectType type) const
{
	std::list<Notify>::const_iterator it = _notifyList.begin();
	for( ;it != _notifyList.end(); ++it )
	{
		if( it->removed ) continue;
		if( it->subscriber->GetType() == type ) return GetRawPtr(it->subscriber);
	}
	return NULL;
}

void GC_Object::OnKillSubscriber(GC_Object *sender, void *param)
{
	Unsubscribe(sender);
}

void GC_Object::PulseNotify(NotyfyType type, void *param)
{
	if( _notifyList.empty() ) return;

	_notifyProtectCount++;

	std::list<Notify>::iterator tmp, it = _notifyList.begin();
	while( it != _notifyList.end() )
	{
		if( type != it->type )
		{
			++it;
			continue;
		}
		assert(it->subscriber);
		(GetRawPtr(it->subscriber)->*it->handler)(this, param);
		tmp = it++;
		if( tmp->once )
		{
			if( tmp->hasGuard )
			{
				// find self in the subscriber's list and remove OnKillSubscriber
				tmp->subscriber->Unguard(this);
				tmp->hasGuard = false;
			}
			_notifyList.erase(tmp);
		}
	}

	if( 0 == --_notifyProtectCount )
	{
		_notifyList.remove_if(Notify::CleanUp());
	}
}

void GC_Object::TimeStepFixed(float dt)
{
}

void GC_Object::TimeStepFloat(float dt)
{
}

void GC_Object::EditorAction()
{
}

SafePtr<PropertySet> GC_Object::GetProperties()
{
	SafePtr<PropertySet> ps(NewPropertySet());
	ps->Exchange(false); // fill property set with data from object
	return ps;
}

PropertySet* GC_Object::NewPropertySet()
{
	return new MyPropertySet(this);
}

void GC_Object::MapExchange(MapFile &f)
{
	string_t tmp_name;
	const char *name = GetName();
	tmp_name = name ? name : "";
	MAP_EXCHANGE_STRING(name, tmp_name, "");

	if( f.loading() )
	{
		SetName(tmp_name.c_str());
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

// ConfigBase.cpp

#include "stdafx.h"
#include "ConfigBase.h"

#include "fs/FileSystem.h"

///////////////////////////////////////////////////////////////////////////////

static ConfVar* GetVarIfTypeMatch(ConfVarTable *parent, const char *key, ConfVar::Type type)
{
	if( ConfVar *v = parent->Find(key) )
	{
		return v->GetType() == type ? v : NULL;
	}
	else if( parent->IsFrozen() )
	{
		return NULL;
	}
	return parent->GetVar(key, type).first;
}

static ConfVar* TableElementFromLua(lua_State *L, ConfVarTable *parent, const char *key)
{
	ConfVar* result = NULL;
	int valueType = lua_type(L, -1);
	switch( valueType )
	{
	case LUA_TSTRING:
		result = GetVarIfTypeMatch(parent, key, ConfVar::typeString);
		break;
	case LUA_TBOOLEAN:
		result = GetVarIfTypeMatch(parent, key, ConfVar::typeBoolean);
		break;
	case LUA_TNUMBER:
		result = GetVarIfTypeMatch(parent, key, ConfVar::typeNumber);
		break;
	case LUA_TTABLE:
		result = GetVarIfTypeMatch(parent, key, lua_objlen(L,-1) ? ConfVar::typeArray : ConfVar::typeTable);
		break;
	default:
		return NULL;
	}

	return result;
}

static ConfVar* ArrayElementFromLua(lua_State *L, ConfVarArray *parent, size_t key)
{
	ConfVar* result = NULL;
	int valueType = lua_type(L, -1);
	switch( valueType )
	{
	case LUA_TSTRING:
		result = parent->GetVar(key, ConfVar::typeString).first;
		break;
	case LUA_TBOOLEAN:
		result = parent->GetVar(key, ConfVar::typeBoolean).first;
		break;
	case LUA_TNUMBER:
		result = parent->GetVar(key, ConfVar::typeNumber).first;
		break;
	case LUA_TTABLE:
		result = parent->GetVar(key, lua_objlen(L,-1) ? ConfVar::typeArray : ConfVar::typeTable).first;
		break;
	default:
		GetConsole().Format(1) << "Unknown lua type - " << lua_typename(L, valueType);
		return NULL;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////

ConfVar::ConfVar()
  : _type(typeNil)
  , _frozen(false)
{
}

ConfVar::~ConfVar()
{
	assert(typeNil == _type);
}

void ConfVar::FireValueUpdate(ConfVar *pVar)
{
	if( eventChange )
		INVOKE(eventChange) ();
}

const char* ConfVar::GetTypeName() const
{
	return "nil";
}

void ConfVar::SetType(Type type)
{
	assert(!_frozen);
	if( type != _type )
	{
		this->~ConfVar(); // manually call the destructor
		switch( type )
		{
#pragma push_macro("new")
#undef new
			case typeNil:     new(this) ConfVar();        break;
			case typeNumber:  new(this) ConfVarNumber();  break;
			case typeBoolean: new(this) ConfVarBool();    break;
			case typeString:  new(this) ConfVarString();  break;
			case typeArray:   new(this) ConfVarArray();   break;
			case typeTable:   new(this) ConfVarTable();   break;
			default: assert(!"unknown ConfVar type");
#pragma pop_macro("new")
		}
		assert( _type == type );
	}
}

ConfVarNumber* ConfVar::AsNum()
{
	assert(typeNumber == _type);
	return static_cast<ConfVarNumber*>(this);
}

ConfVarBool* ConfVar::AsBool()
{
	assert(typeBoolean == _type);
	return static_cast<ConfVarBool*>(this);
}

ConfVarString* ConfVar::AsStr()
{
	assert(typeString == _type);
	return static_cast<ConfVarString*>(this);
}

ConfVarArray* ConfVar::AsArray()
{
	assert(typeArray == _type);
	return static_cast<ConfVarArray*>(this);
}

ConfVarTable* ConfVar::AsTable()
{
	assert(typeTable == _type);
	return static_cast<ConfVarTable*>(this);
}

bool ConfVar::Write(FILE *, int) const
{
	assert(false);
	return false;
}

bool ConfVar::Assign(lua_State *)
{
	assert(false);
	return false;
}

void ConfVar::Push(lua_State *) const
{
	assert(false);
}

void ConfVar::Freeze(bool freeze)
{
	_frozen = freeze;
}


///////////////////////////////////////////////////////////////////////////////
// number

ConfVarNumber::ConfVarNumber()
{
	_type = typeNumber;
	_val.asNumber = 0;
}

ConfVarNumber::~ConfVarNumber()
{
	_type = typeNil;
}

const char* ConfVarNumber::GetTypeName() const
{
	return "number";
}

double ConfVarNumber::GetRawNumber() const
{
	assert(typeNumber == _type);
	return _val.asNumber;
}

void ConfVarNumber::SetRawNumber(double value)
{
	assert(typeNumber == _type);
	if( _val.asNumber != value )
	{
		_val.asNumber = value;
		FireValueUpdate(this);
	}
}

float ConfVarNumber::GetFloat() const
{
	assert(typeNumber == _type);
	return (float) _val.asNumber;
}
void ConfVarNumber::SetFloat(float value)
{
	assert(typeNumber == _type);
	if( _val.asNumber != value )
	{
		_val.asNumber = value;
		FireValueUpdate(this);
	}
}

int ConfVarNumber::GetInt() const
{
	assert(typeNumber == _type);
	return (int) _val.asNumber;
}
void ConfVarNumber::SetInt(int value)
{
	assert(typeNumber == _type);
	if( _val.asNumber != value )
	{
		_val.asNumber = value;
		FireValueUpdate(this);
	}
}

bool ConfVarNumber::Write(FILE *file, int indent) const
{
	fprintf(file, "%.10g", _val.asNumber);
	return true;
}

bool ConfVarNumber::Assign(lua_State *L)
{
	SetFloat( (float) lua_tonumber(L, -1) );
	return true;
}

void ConfVarNumber::Push(lua_State *L) const
{
	lua_pushnumber(L, GetFloat());
}

///////////////////////////////////////////////////////////////////////////////
// boolean

ConfVarBool::ConfVarBool()
{
	_type = typeBoolean;
	_val.asBool = false;
}

ConfVarBool::~ConfVarBool()
{
	_type = typeNil;
}

const char* ConfVarBool::GetTypeName() const
{
	return "boolean";
}

bool ConfVarBool::Get() const
{
	assert(typeBoolean == _type);
	return _val.asBool;
}
void ConfVarBool::Set(bool value)
{
	assert(typeBoolean == _type);
	_val.asBool = value;
	FireValueUpdate(this);
}

bool ConfVarBool::Write(FILE *file, int indent) const
{
	fprintf(file, Get() ? "true" : "false");
	return true;
}

bool ConfVarBool::Assign(lua_State *L)
{
	Set( 0 != lua_toboolean(L, -1) );
	return true;
}

void ConfVarBool::Push(lua_State *L) const
{
	lua_pushboolean(L, Get());
}

///////////////////////////////////////////////////////////////////////////////
// string

ConfVarString::ConfVarString()
{
	_type = typeString;
	_val.asString = new string_t();
}

ConfVarString::~ConfVarString()
{
	assert( typeString == _type );
	delete _val.asString;
	_type = typeNil;
}

const char* ConfVarString::GetTypeName() const
{
	return "string";
}

const string_t& ConfVarString::Get() const
{
	assert(typeString == _type);
	return *_val.asString;
}

void ConfVarString::Set(const string_t &value)
{
	assert(typeString == _type);
	*_val.asString = value;
	FireValueUpdate(this);
}

bool ConfVarString::Write(FILE *file, int indent) const
{
	fputc('"', file);
	for( size_t i = 0; i < _val.asString->size(); ++i )
	{
		const int c = (*_val.asString)[i];
		switch(c)
		{
		case '\\':
			fputs("\\\\", file);
			break;
		case '\n':
			fputs("\\n", file);
			break;
		case '"':
			fputs("\\\"", file);
			break;
		case '\t':
			fputs("\\t", file);
			break;
		default:
			fputc(c, file);
		}
	}
	fputc('"', file);
	return true;
}

bool ConfVarString::Assign(lua_State *L)
{
	Set(lua_tostring(L, -1));
	return true;
}

void ConfVarString::Push(lua_State *L) const
{
	lua_pushstring(L, Get().c_str());
}


///////////////////////////////////////////////////////////////////////////////
// array

ConfVarArray::ConfVarArray()
{
	_type = typeArray;
	_val.asArray = new std::deque<ConfVar*>();
}

ConfVarArray::~ConfVarArray()
{
	assert( typeArray == _type );
	for( size_t i = 0; i < _val.asArray->size(); ++i )
	{
		delete (*_val.asArray)[i];
	}
	delete _val.asArray;
	_type = typeNil;
}

const char* ConfVarArray::GetTypeName() const
{
	return "array";
}

std::pair<ConfVar*, bool> ConfVarArray::GetVar(size_t index, ConfVar::Type type)
{
	assert( index < GetSize() );
	std::pair<ConfVar*, bool> result( (*_val.asArray)[index], true);

	if( result.first->GetType() != type )
	{
		result.first->SetType(type);
		result.second = false;
		FireValueUpdate(this);
		assert(result.first->GetType() == type);
	}

	return result;
}

ConfVarNumber* ConfVarArray::GetNum(size_t index, float def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetFloat(def);
	return p.first->AsNum();
}

ConfVarNumber* ConfVarArray::SetNum(size_t index, float value)
{
	ConfVarNumber *v = GetVar(index, ConfVar::typeNumber).first->AsNum();
	v->SetFloat(value);
	return v;
}

ConfVarNumber* ConfVarArray::GetNum(size_t index, int def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetInt(def);
	return p.first->AsNum();
}

ConfVarNumber* ConfVarArray::SetNum(size_t index, int value)
{
	ConfVarNumber *v = GetVar(index, ConfVar::typeNumber).first->AsNum();
	v->SetInt(value);
	return v;
}

ConfVarBool* ConfVarArray::GetBool(size_t index, bool def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeBoolean);
	if( !p.second )
		p.first->AsBool()->Set(def);
	return p.first->AsBool();
}

ConfVarBool* ConfVarArray::SetBool(size_t index, bool value)
{
	ConfVarBool *v = GetVar(index, ConfVar::typeBoolean).first->AsBool();
	v->Set(value);
	return v;
}

ConfVarString* ConfVarArray::GetStr(size_t index, const string_t &def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeString);
	if( !p.second )
		p.first->AsStr()->Set(def);
	return p.first->AsStr();
}

ConfVarString* ConfVarArray::SetStr(size_t index, const string_t &value)
{
	ConfVarString *v = GetVar(index, ConfVar::typeString).first->AsStr();
	v->Set(value);
	return v;
}

ConfVarArray* ConfVarArray::GetArray(size_t index)
{
	return GetVar(index, ConfVar::typeArray).first->AsArray();
}

ConfVarTable* ConfVarArray::GetTable(size_t index)
{
	return GetVar(index, ConfVar::typeTable).first->AsTable();
}

void ConfVarArray::Resize(size_t newSize)
{
	assert(typeArray == _type);
	size_t oldSize = _val.asArray->size();
	if( newSize > oldSize )
	{
		_val.asArray->resize(newSize);
		for( size_t i = oldSize; i < newSize; ++i )
		{
			(*_val.asArray)[i] = new ConfVar();
		}
		FireValueUpdate(this);
	}
	else if( newSize < oldSize )
	{
		for( size_t i = newSize; i < oldSize; ++i )
		{
			delete (*_val.asArray)[i];
		}
		_val.asArray->resize(newSize);
		FireValueUpdate(this);
	}
}

size_t ConfVarArray::GetSize() const
{
	assert(typeArray == _type);
	return _val.asArray->size();
}

ConfVar* ConfVarArray::GetAt(size_t index) const
{
	assert(typeArray == _type);
	return (*_val.asArray)[index];
}

void ConfVarArray::RemoveAt(size_t index)
{
	assert(typeArray == _type);
	delete (*_val.asArray) [index];
	_val.asArray->erase(_val.asArray->begin() + index);
	FireValueUpdate(this);
}

void ConfVarArray::PopFront()
{
	assert(typeArray == _type);
	delete _val.asArray->front();
	_val.asArray->pop_front();
	FireValueUpdate(this);
}

ConfVar* ConfVarArray::PushBack(Type type)
{
	assert(typeArray == _type);
	ConfVar *result = new ConfVar();
	result->SetType(type);
	_val.asArray->push_back(result);
	FireValueUpdate(this);
	return result;
}

void ConfVarArray::PopBack()
{
	assert(typeArray == _type);
	delete _val.asArray->back();
	_val.asArray->pop_back();
	FireValueUpdate(this);
}

ConfVar* ConfVarArray::PushFront(Type type)
{
	assert(typeArray == _type);
	ConfVar *result = new ConfVar();
	result->SetType(type);
	_val.asArray->push_front(result);
	FireValueUpdate(this);
	return result;
}

bool ConfVarArray::Write(FILE *file, int indent) const
{
	fprintf(file, "{\n");
	bool delim = false;
	for( size_t i = 0; i < GetSize(); ++i )
	{
		if( delim ) fprintf(file, ",\n");
		for( int n = 0; n < indent; ++n )
		{
			fprintf(file, "  ");
		}
		delim = true;
		if( !GetAt(i)->Write(file, indent+1) )
			return false;
	}
	fprintf(file, "\n}");
	return true;
}

bool ConfVarArray::Assign(lua_State *L)
{
	Resize( lua_objlen(L, -1) );

	for( size_t i = 0; i < GetSize(); ++i )
	{
		lua_pushinteger(L, i+1); // push the key
		lua_gettable(L, -2);   // pop the key, push the value

		if( ConfVar *v = ArrayElementFromLua(L, this, i) )
		{
			if( !v->Assign(L) )
				return false;
		}

		lua_pop(L, 1);         // pop the value
	}

	return true;
}

void ConfVarArray::Push(lua_State *L) const
{
	*reinterpret_cast<ConfVarArray const**>( lua_newuserdata(L, sizeof(this)) ) = this;
	luaL_getmetatable(L, "conf_array");  // metatable for config
	lua_setmetatable(L, -2);
}


///////////////////////////////////////////////////////////////////////////////
// table

ConfVarTable::ConfVarTable()
{
	_type = typeTable;
	_val.asTable = new std::map<string_t, ConfVar*>();
}

ConfVarTable::~ConfVarTable()
{
	assert(typeTable == _type);
	ClearInternal();
	delete _val.asTable;
	_type = typeNil;
}

void ConfVarTable::ClearInternal()
{
	assert(typeTable == _type);
	for( std::map<string_t, ConfVar*>::iterator it = _val.asTable->begin(); _val.asTable->end() != it; ++it )
	{
		delete it->second;
	}
	_val.asTable->clear();
}

const char* ConfVarTable::GetTypeName() const
{
	return "table";
}

ConfVar* ConfVarTable::Find(const string_t &name)  // returns NULL if variable not found
{
	std::map<string_t, ConfVar*>::iterator it = _val.asTable->find(name);
	return _val.asTable->end() != it ? it->second : NULL;
}

size_t ConfVarTable::GetSize() const
{
	return _val.asTable->size();
}

void ConfVarTable::GetKeyList(std::vector<string_t> &out) const
{
	out.clear();
	for( std::map<string_t, ConfVar*>::const_iterator it = _val.asTable->begin();
		_val.asTable->end() != it; ++it )
	{
		out.push_back(it->first);
	}
}

std::pair<ConfVar*, bool> ConfVarTable::GetVar(const string_t &name, ConfVar::Type type)
{
	std::pair<ConfVar*, bool> result(NULL, true);

	assert(ConfVar::typeNil != type);
	std::map<string_t, ConfVar*>::iterator it = _val.asTable->find(name);
	if( _val.asTable->end() == it )
	{
		// create new item
		assert( !_frozen );
		result.first = new ConfVar();
		std::pair<string_t, ConfVar*> pair(name, result.first);
		_val.asTable->insert(pair);
		FireValueUpdate(this);
	}
	else
	{
		result.first = it->second;
	}

	if( result.first->GetType() != type )
	{
		// change type of the existing item
		assert( !_frozen );
		result.first->SetType(type);
		result.second = false;
		FireValueUpdate(this);
	}
	assert(result.first->GetType() == type);

	return result;
}

ConfVarNumber* ConfVarTable::GetNum(const string_t &name, float def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetFloat(def);
	return p.first->AsNum();
}

ConfVarNumber* ConfVarTable::SetNum(const string_t &name, float value)
{
	ConfVarNumber *v = GetVar(name, ConfVar::typeNumber).first->AsNum();
	v->SetFloat(value);
	return v;
}

ConfVarNumber* ConfVarTable::GetNum(const string_t &name, int def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetInt(def);
	return p.first->AsNum();
}

ConfVarNumber* ConfVarTable::SetNum(const string_t &name, int value)
{
	ConfVarNumber *v = GetVar(name, ConfVar::typeNumber).first->AsNum();
	v->SetInt(value);
	return v;
}

ConfVarBool* ConfVarTable::GetBool(const string_t &name, bool def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeBoolean);
	if( !p.second )
		p.first->AsBool()->Set(def);
	return p.first->AsBool();
}

ConfVarBool* ConfVarTable::SetBool(const string_t &name, bool value)
{
	ConfVarBool *v = GetVar(name, ConfVar::typeBoolean).first->AsBool();
	v->Set(value);
	return v;
}

ConfVarString* ConfVarTable::GetStr(const string_t &name, const string_t &def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeString);
	if( !p.second )
		p.first->AsStr()->Set(def);
	return p.first->AsStr();
}

ConfVarString* ConfVarTable::SetStr(const string_t &name, const string_t &value)
{
	ConfVarString *v = GetVar(name, ConfVar::typeString).first->AsStr();
	v->Set(value);
	return v;
}

ConfVarArray* ConfVarTable::GetArray(const string_t &name, void (*init)(ConfVarArray*))
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeArray);
	if( !p.second && init )
		init(p.first->AsArray());
	return p.first->AsArray();
}

ConfVarTable* ConfVarTable::GetTable(const string_t &name, void (*init)(ConfVarTable*))
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeTable);
	if( !p.second && init )
		init(p.first->AsTable());
	return p.first->AsTable();
}

void ConfVarTable::Clear()
{
	assert( typeTable == _type );
	assert( !_frozen );
	ClearInternal();
	FireValueUpdate(this);
}

bool ConfVarTable::Remove(ConfVar * const value)
{
	assert( typeTable == _type );
	assert( !_frozen );
	for( std::map<string_t, ConfVar*>::iterator it = _val.asTable->begin();
		_val.asTable->end() != it; ++it )
	{
		if( value == it->second )
		{
			delete it->second;
			_val.asTable->erase(it);
			FireValueUpdate(this);
			return true;
		}
	}
	return false;
}

bool ConfVarTable::Remove(const string_t &name)
{
	assert( typeTable == _type );
	assert( !_frozen );
	std::map<string_t, ConfVar*>::iterator it = _val.asTable->find(name);
	if( _val.asTable->end() != it )
	{
		_val.asTable->erase(it);
		FireValueUpdate(this);
		return true;
	}
	return false;
}

bool ConfVarTable::Rename(ConfVar * const value, const string_t &newName)
{
	assert( typeTable == _type );
	assert( !_frozen );

	std::map<string_t, ConfVar*>::iterator it = _val.asTable->begin();
	for( ;_val.asTable->end() != it; ++it )
	{
		if( value == it->second )
		{
			break;
		}
	}
	if( _val.asTable->end() == it )
	{
		return false; // old name not found
	}

	if( it->first == newName )
	{
		return true;
	}

	if( _val.asTable->count(newName) )
	{
		return false; // new name is already used
	}

	(*_val.asTable)[newName] = it->second;   // create new
	_val.asTable->erase(it);                 // remove old
	FireValueUpdate(this);

	return true;
}

bool ConfVarTable::Rename(const string_t &oldName, const string_t &newName)
{
	assert( typeTable == _type );
	assert( !_frozen );

	std::map<string_t, ConfVar*>::iterator it = _val.asTable->find(oldName);
	if( _val.asTable->end() == it )
	{
		return false; // old name not found
	}

	if( it->first == newName )
	{
		return true;
	}

	if( _val.asTable->count(newName) )
	{
		return false; // new name is already used
	}

	(*_val.asTable)[newName] = it->second;   // create new
	_val.asTable->erase(it);                 // remove old
	FireValueUpdate(this);

	return true;
}

bool ConfVarTable::Write(FILE *file, int indent) const
{
	if( indent ) fprintf(file, "{%s\n", GetHelpString().empty() ? "" : (string_t(" -- ") + GetHelpString()).c_str());

	for( std::map<string_t, ConfVar*>::const_iterator it = _val.asTable->begin();
	     _val.asTable->end() != it; ++it )
	{
		for( int i = 0; i < indent; ++i )
		{
			fputs("  ", file);
		}

		bool safe = true;
		for( size_t i = 0; i < it->first.size(); ++i )
		{
			unsigned char c = it->first[i];
			if( !isalpha(c) && '_' != c )
			{
				if( 0 == i || !isdigit(c) )
				{
					safe = false;
					break;
				}
			}
		}

		if( safe )
		{
			fputs(it->first.c_str(), file);
		}
		else
		{
			fputs("[\"", file);
			for( size_t i = 0; i < it->first.size(); ++i )
			{
				const int c = it->first[i];
				switch(c)
				{
				case '\\':
					fputs("\\\\", file);
					break;
				case '\n':
					fputs("\\n", file);
					break;
				case '"':
					fputs("\\\"", file);
					break;
				case '\t':
					fputs("\\t", file);
					break;
				default:
					fputc(c, file);
				}
			}
			fputs("\"]", file);
		}

		fputs(" = ", file);

		if( !it->second->Write(file, indent+1) )
			return false;

		fputs(indent ? ",":";", file);

		// help string
		if( !it->second->GetHelpString().empty() )
		{
			fputs("  -- ", file);
			fputs(it->second->GetHelpString().c_str(), file);
		}
		fputs("\n", file);
	}
	if( indent )
	{
		for( int i = 0; i < indent-1; ++i )
		{
			fputs("  ", file);
		}
		fputs("}", file);
	}

	return true;
}

bool ConfVarTable::Assign(lua_State *L)
{
	// enumerate all fields of the table
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		// use only string keys
		if( LUA_TSTRING == lua_type(L, -2) )
		{
			const char *key = lua_tostring(L, -2);
			if( ConfVar *v = TableElementFromLua(L, this, key) )
			{
				if( !v->Assign(L) )
					return false;
			}
			else
			{
				GetConsole().Printf(1, "variable '%s' was dropped", key);
			}
		}
	}

	return true;
}

bool ConfVarTable::Save(const char *filename) const
{
	FILE *file = NULL;
	errno_t err = fopen_s(&file, filename, "w");
	if( !file )
	{
		return false;
	}
	fprintf(file, "-- config file was automatically generated by application\n\n");
	bool result = Write(file, 0);
	fclose(file);
	return result;
}

bool ConfVarTable::Load(const char *filename)
{
	lua_State *L = lua_open();

	// try to read and execute the file
	if( luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0) )
	{
		GetConsole().WriteLine(1, lua_tostring(L, -1));
		lua_close(L);
		return false;
	}

	// get global table
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	bool result = Assign(L);

	lua_close(L);

	FireValueUpdate(this);

	return result;
}

void ConfVarTable::Push(lua_State *L) const
{
	*reinterpret_cast<ConfVarTable const**>(lua_newuserdata(L, sizeof(this))) = this;
	luaL_getmetatable(L, "conf_table");  // metatable for config
	lua_setmetatable(L, -2);
}


///////////////////////////////////////////////////////////////////////////////
// lua binding

// returns type string for arrays and tables
static int luaT_conftostring(lua_State *L)
{
	assert(lua_type(L, 1) == LUA_TUSERDATA);
	ConfVar *v = *reinterpret_cast<ConfVar **>( lua_touserdata(L, 1) );
	switch( v->GetType() )
	{
	case ConfVar::typeArray:
		lua_pushfstring(L, "conf_array: %p", v);
		break;
	case ConfVar::typeTable:
		lua_pushfstring(L, "conf_table: %p", v);
		break;
	default:
		assert(FALSE);
	}
	return 1;
}

// retrieving an array element
static int luaT_getconfarray(lua_State *L)
{
	lua_settop(L, 2);

	ConfVarArray *v = *reinterpret_cast<ConfVarArray **>(luaL_checkudata(L, 1, "conf_array"));
	assert( ConfVar::typeArray == v->GetType() );

	size_t index = luaL_checkinteger(L, 2);
	if( index >= v->AsArray()->GetSize() )
	{
		return luaL_error(L, "array index is out of range");
	}
	v->GetAt(index)->Push(L);
	return 1;
}

// retrieving a table element
static int luaT_getconftable(lua_State *L)
{
	lua_settop(L, 2);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>(luaL_checkudata(L, 1, "conf_table"));
	assert( ConfVar::typeTable == v->GetType() );

	const char *key = luaL_checkstring(L, 2);

	if( ConfVar *result = v->Find(key) )
	{
		result->Push(L);
	}
	else
	{
		return luaL_error(L, "variable '%s' not found", key);
	}

	return 1;
}

// assigning a value to an array element
static int luaT_setconfarray(lua_State *L)
{
	lua_settop(L, 3);

	ConfVarArray *v = *reinterpret_cast<ConfVarArray **>(luaL_checkudata(L, 1, "conf_array"));
	assert( ConfVar::typeArray == v->GetType() );

	size_t index = luaL_checkinteger(L, 2);

	if( index >= v->AsArray()->GetSize() )
	{
		return luaL_error(L, "array index is out of range");
	}

	ConfVar *val = v->GetAt(index);
	switch( val->GetType() )
	{
	case ConfVar::typeBoolean:
		val->AsBool()->Set( 0 != lua_toboolean(L, 3) );
		break;
	case ConfVar::typeNumber:
		val->AsNum()->SetFloat( (float) luaL_checknumber(L, 3) );
		break;
	case ConfVar::typeString:
		val->AsStr()->Set(luaL_checkstring(L, 3));
		break;
	case ConfVar::typeArray:
		return luaL_error(L, "attempt to modify conf_array");
	case ConfVar::typeTable:
		return luaL_error(L, "attempt to modify conf_table");
	}

	return 0;
}

// assigning a value to a table element
static int luaT_setconftable(lua_State *L)
{
	lua_settop(L, 3);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>(luaL_checkudata(L, 1, "conf_table"));
	assert( ConfVar::typeTable == v->GetType() );

	const char *key = luaL_checkstring(L, 2);

	if( ConfVar *val = v->Find(key) )
	{
		switch( val->GetType() )
		{
		case ConfVar::typeBoolean:
			val->AsBool()->Set(0 != lua_toboolean(L, 3));
			break;
		case ConfVar::typeNumber:
			val->AsNum()->SetFloat( (float) luaL_checknumber(L, 3) );
			break;
		case ConfVar::typeString:
			val->AsStr()->Set( luaL_checkstring(L, 3) );
			break;
		case ConfVar::typeArray:
			luaL_checktype(L, 3, LUA_TTABLE);
			return val->Assign(L);
		case ConfVar::typeTable:
			luaL_checktype(L, 3, LUA_TTABLE);
			return luaL_error(L, "attempt to modify conf_table");
		}
	}
	else
	{
		return luaL_error(L, "variable '%s' not found", key);
	}
	return 0;
}

// generic __next support for tables
int ConfVarTable::luaT_conftablenext(lua_State *L)
{
	lua_settop(L, 2);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>(luaL_checkudata(L, 1, "conf_table"));
	assert( ConfVar::typeTable == v->GetType() );

	if( v->_val.asTable->empty() )
	{
		lua_pushnil(L);
		return 1;
	}

	if( lua_isnil(L, 2) )
	{
		// begin iteration
		lua_pushstring(L, v->_val.asTable->begin()->first.c_str()); // key
		v->_val.asTable->begin()->second->Push(L);                  // value
		return 2;
	}

	const char *key = luaL_checkstring(L, 2);
	std::map<string_t, ConfVar*>::const_iterator it = v->_val.asTable->find(key);
	if( v->_val.asTable->end() == it )
	{
		return luaL_error(L, "invalid key to 'next'");
	}

	++it;

	if( v->_val.asTable->end() == it )
	{
		// end of list
		lua_pushnil(L);
		return 1;
	}

	// return next pair
	lua_pushstring(L, it->first.c_str());   // key
	it->second->Push(L);                    // value
	return 2;
}


// map config to the conf lua variable
void ConfVarTable::InitConfigLuaBinding(lua_State *L, const char *globName)
{
	int top = lua_gettop(L);

	luaL_newmetatable(L, "conf_table");  // metatable for tables
	 lua_pushcfunction(L, luaT_setconftable);
	  lua_setfield(L, -2, "__newindex");
	 lua_pushcfunction(L, luaT_getconftable);
	  lua_setfield(L, -2, "__index");
	 lua_pushcfunction(L, luaT_conftostring);
	  lua_setfield(L, -2, "__tostring");
	 lua_pushcfunction(L, luaT_conftablenext);
	  lua_setfield(L, -2, "__next");
	 lua_pop(L, 1); // pop the metatable

	luaL_newmetatable(L, "conf_array");  // metatable for arrays
	 lua_pushcfunction(L, luaT_setconfarray);  // push handler function
	  lua_setfield(L, -2, "__newindex");        // this also pops function from the stack
	 lua_pushcfunction(L, luaT_getconfarray);  // push handler function
	  lua_setfield(L, -2, "__index");           // this also pops function from the stack
	 lua_pushcfunction(L, luaT_conftostring);  // push handler function
	  lua_setfield(L, -2, "__tostring");        // this also pops function from the stack
	 lua_pop(L, 1); // pop the metatable

	Push(L);
	 lua_setglobal(L, globName);    // set global and pop one element from stack

	assert(lua_gettop(L) == top);
}

///////////////////////////////////////////////////////////////////////////////

LuaConfigCacheBase::LuaConfigCacheBase()
{
}

LuaConfigCacheBase::~LuaConfigCacheBase()
{
}

LuaConfigCacheBase::helper* LuaConfigCacheBase::operator ->()
{
	return &_helper;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

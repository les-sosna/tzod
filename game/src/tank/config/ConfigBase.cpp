// ConfigBase.cpp

#include "stdafx.h"
#include "ConfigBase.h"

#include "core/Console.h"

///////////////////////////////////////////////////////////////////////////////

template< class key_t, class parent_t >
static ConfVar* FromLuaType(lua_State *L, parent_t *parent, key_t key)
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
		g_console->printf("WARNING: unknown lua type - %s\n", lua_typename(L, valueType));
		return NULL;
	}

	return result;
}

///////////////////////////////////////////////////////////////////////////////

ConfVar::ConfVar()
{
	_type    = typeNil;
	_val.ptr = NULL;
}

ConfVar::~ConfVar()
{
	_type    = typeNil;
	_val.ptr = NULL;
}

const char* ConfVar::GetTypeName() const
{
	return "nil";
}

void ConfVar::SetType(Type type)
{
	if( type != _type )
	{
		this->~ConfVar(); // manually call the destructor
		switch( type )
		{
			case typeNil:     new( this ) ConfVar();        break;
			case typeNumber:  new( this ) ConfVarNumber();  break;
			case typeBoolean: new( this ) ConfVarBool();    break;
			case typeString:  new( this ) ConfVarString();  break;
			case typeArray:   new( this ) ConfVarArray();   break;
			case typeTable:   new( this ) ConfVarTable();   break;
			default: _ASSERT(FALSE);
		}
		_ASSERT( _type == type );
	}
}

ConfVarNumber* ConfVar::AsNum()
{
	_ASSERT(typeNumber == _type);
	return static_cast<ConfVarNumber*>(this);
}

ConfVarBool* ConfVar::AsBool()
{
	_ASSERT(typeBoolean == _type);
	return static_cast<ConfVarBool*>(this);
}

ConfVarString* ConfVar::AsStr()
{
	_ASSERT(typeString == _type);
	return static_cast<ConfVarString*>(this);
}

ConfVarArray* ConfVar::AsArray()
{
	_ASSERT(typeArray == _type);
	return static_cast<ConfVarArray*>(this);
}

ConfVarTable* ConfVar::AsTable()
{
	_ASSERT(typeTable == _type);
	return static_cast<ConfVarTable*>(this);
}

bool ConfVar::_Save(FILE *file, int level) const
{
	return true;
}

bool ConfVar::_Load(lua_State *L)
{
	return true;
}

void ConfVar::Push(lua_State *L)
{
	lua_pushnil(L);
}

///////////////////////////////////////////////////////////////////////////////
// number

ConfVarNumber::ConfVarNumber()
{
	_val.asNumber = 0;
	_type = typeNumber;
}

const char* ConfVarNumber::GetTypeName() const
{
	return "number";
}

float ConfVarNumber::GetFloat() const
{
	_ASSERT(typeNumber == _type);
	return (float) _val.asNumber;
}
void ConfVarNumber::SetFloat(float value)
{
	_ASSERT(typeNumber == _type);
	_val.asNumber = value;
	if( eventChange )
		INVOKE(eventChange) ();
}

int ConfVarNumber::GetInt() const
{
	_ASSERT(typeNumber == _type);
	return (int) _val.asNumber;
}
void ConfVarNumber::SetInt(int value)
{
	_ASSERT(typeNumber == _type);
	_val.asNumber = value;
	if( eventChange )
		INVOKE(eventChange) ();
}

bool ConfVarNumber::_Save(FILE *file, int level) const
{
	fprintf(file, "%g", GetFloat());
	return true;
}

bool ConfVarNumber::_Load(lua_State *L)
{
	SetFloat( (float) lua_tonumber(L, -1) );
	return true;
}

void ConfVarNumber::Push(lua_State *L)
{
	lua_pushnumber(L, GetFloat());
}

///////////////////////////////////////////////////////////////////////////////
// boolean

ConfVarBool::ConfVarBool()
{
	_val.asBool = false;
	_type = typeBoolean;
}

const char* ConfVarBool::GetTypeName() const
{
	return "boolean";
}

bool ConfVarBool::Get() const
{
	_ASSERT(typeBoolean == _type);
	return _val.asBool;
}
void ConfVarBool::Set(bool value)
{
	_ASSERT(typeBoolean == _type);
	_val.asBool = value;
	if( eventChange )
		INVOKE(eventChange) ();
}

bool ConfVarBool::_Save(FILE *file, int level) const
{
	fprintf(file, Get() ? "true" : "false");
	return true;
}

bool ConfVarBool::_Load(lua_State *L)
{
	Set( 0 != lua_toboolean(L, -1) );
	return true;
}

void ConfVarBool::Push(lua_State *L)
{
	lua_pushboolean(L, Get());
}

///////////////////////////////////////////////////////////////////////////////
// string

ConfVarString::ConfVarString()
{
	_val.asString = new string_t();
	_type = typeString;
}

ConfVarString::~ConfVarString()
{
	_ASSERT( typeString == _type );
	delete _val.asString;
}

const char* ConfVarString::GetTypeName() const
{
	return "string";
}

const char* ConfVarString::Get() const
{
	_ASSERT(typeString == _type);
	return _val.asString->c_str();
}

void ConfVarString::Set(const char* value)
{
	_ASSERT(typeString == _type);
	*_val.asString = value;
	if( eventChange )
		INVOKE(eventChange) ();
}

bool ConfVarString::_Save(FILE *file, int level) const
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

bool ConfVarString::_Load(lua_State *L)
{
	Set(lua_tostring(L, -1));
	return true;
}

void ConfVarString::Push(lua_State *L)
{
	lua_pushstring(L, Get());
}


///////////////////////////////////////////////////////////////////////////////
// array

ConfVarArray::ConfVarArray()
{
	_val.asArray = new std::deque<ConfVar*>();
	_type = typeArray;
}

ConfVarArray::~ConfVarArray()
{
	_ASSERT( typeArray == _type );
	for( size_t i = 0; i < _val.asArray->size(); ++i )
	{
		delete (*_val.asArray)[i];
	}
	delete _val.asArray;
}

const char* ConfVarArray::GetTypeName() const
{
	return "array";
}

std::pair<ConfVar*, bool> ConfVarArray::GetVar(size_t index, ConfVar::Type type)
{
	_ASSERT( index < GetSize() );
	std::pair<ConfVar*, bool> result( (*_val.asArray)[index], true);

	if( result.first->GetType() != type )
	{
		const bool warn = result.first->GetType() != ConfVar::typeNil;
		const char *typeName = result.first->GetTypeName();

		result.first->SetType(type);
		result.second = false;

		if( warn )
		{
			g_console->printf("WARNING: changing type of element with index %u from %s to %s\n",
				index, typeName, result.first->GetTypeName() );
		}
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

ConfVarString* ConfVarArray::GetStr(size_t index, const char* def)
{
	std::pair<ConfVar*, bool> p = GetVar(index, ConfVar::typeString);
	if( !p.second )
		p.first->AsStr()->Set(def);
	return p.first->AsStr();
}

ConfVarString* ConfVarArray::SetStr(size_t index, const char* value)
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
	_ASSERT(typeArray == _type);

	size_t oldSize = _val.asArray->size();

	if( newSize > oldSize )
	{
		_val.asArray->resize(newSize);
		for( size_t i = oldSize; i < newSize; ++i )
		{
			(*_val.asArray)[i] = new ConfVar();
		}

//		if( eventChange )
//			INVOKE(eventChange) ();
	}
	else
	if( newSize < oldSize )
	{
		for( size_t i = newSize; i < oldSize; ++i )
		{
			delete (*_val.asArray)[i];
		}
		_val.asArray->resize(newSize);

//		if( eventChangeValue )
//			INVOKE(eventChangeValue) ();
	}
}

size_t ConfVarArray::GetSize() const
{
	_ASSERT(typeArray == _type);
	return _val.asArray->size();
}

ConfVar* ConfVarArray::GetAt(size_t index) const
{
	_ASSERT(typeArray == _type);
	return (*_val.asArray)[index];
}

void ConfVarArray::RemoveAt(size_t index)
{
	_ASSERT(typeArray == _type);
	delete (*_val.asArray) [index];
	_val.asArray->erase(_val.asArray->begin() + index);
}

void ConfVarArray::PopFront()
{
	_ASSERT(typeArray == _type);
	delete _val.asArray->front();
	_val.asArray->pop_front();
}

ConfVar* ConfVarArray::PushBack(Type type)
{
	_ASSERT(typeArray == _type);
	ConfVar *result = new ConfVar();
	result->SetType(type);
	_val.asArray->push_back(result);
	return result;
}

void ConfVarArray::PopBack()
{
	_ASSERT(typeArray == _type);
	delete _val.asArray->back();
	_val.asArray->pop_back();
}

ConfVar* ConfVarArray::PushFront(Type type)
{
	_ASSERT(typeArray == _type);
	ConfVar *result = new ConfVar();
	result->SetType(type);
	_val.asArray->push_front(result);
	return result;
}

bool ConfVarArray::_Save(FILE *file, int level) const
{
	fprintf(file, "{\n");
	bool delim = false;
	for( size_t i = 0; i < GetSize(); ++i )
	{
		if( delim ) fprintf(file, ",\n");
		for( int n = 0; n < level; ++n )
		{
			fprintf(file, "  ");
		}
		delim = true;
		if( !GetAt(i)->_Save(file, level+1) )
			return false;
	}
	fprintf(file, "\n}");
	return true;
}

bool ConfVarArray::_Load(lua_State *L)
{
	Resize( lua_objlen(L, -1) );

	for( size_t i = 0; i < GetSize(); ++i )
	{
		lua_pushinteger(L, i+1); // push the key
		lua_gettable(L, -2);   // pop the key, push the value

		if( ConfVar *v = FromLuaType(L, this, i) )
		{
			if( !v->_Load(L) )
				return false;
		}

		lua_pop(L, 1);         // pop the value
	}

	return true;
}

void ConfVarArray::Push(lua_State *L)
{
	*reinterpret_cast<ConfVarArray**>( lua_newuserdata(L, sizeof(this)) ) = this;
	luaL_getmetatable(L, "conf_array");  // metatable for config
	lua_setmetatable(L, -2);
}


///////////////////////////////////////////////////////////////////////////////
// table

ConfVarTable::ConfVarTable()
{
	_val.asTable = new std::map<string_t, ConfVar*>();
	_type = typeTable;
}

ConfVarTable::~ConfVarTable()
{
	_ASSERT( typeTable == _type );
	for( std::map<string_t, ConfVar*>::iterator it = _val.asTable->begin();
	     _val.asTable->end() != it; ++it )
	{
		delete it->second;
	}
	delete _val.asTable;
}

const char* ConfVarTable::GetTypeName() const
{
	return "table";
}

ConfVar* ConfVarTable::Find(const char *name)  // returns NULL if variable not found
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

std::pair<ConfVar*, bool> ConfVarTable::GetVar(const char *name, ConfVar::Type type)
{
	std::pair<ConfVar*, bool> result(NULL, true);

	_ASSERT( ConfVar::typeNil != type );
	std::map<string_t, ConfVar*>::iterator it = _val.asTable->find(name);
	if( _val.asTable->end() == it )
	{
		result.first = new ConfVar();
		std::pair<string_t, ConfVar*> pair(name, result.first);
		_val.asTable->insert(pair);
	}
	else
	{
		result.first = it->second;
	}

	if( result.first->GetType() != type )
	{
		const bool warn = result.first->GetType() != ConfVar::typeNil;
		const char *typeName = result.first->GetTypeName();

		result.first->SetType(type);
		result.second = false;

		if( warn )
		{
			g_console->printf("WARNING: changing type of variable '%s' from %s to %s\n",
				name, typeName, result.first->GetTypeName() );
		}
	}

	return result;
}

ConfVarNumber* ConfVarTable::GetNum(const char *name, float def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetFloat(def);
	return p.first->AsNum();
}

ConfVarNumber* ConfVarTable::SetNum(const char *name, float value)
{
	ConfVarNumber *v = GetVar(name, ConfVar::typeNumber).first->AsNum();
	v->SetFloat(value);
	return v;
}

ConfVarNumber* ConfVarTable::GetNum(const char *name, int def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetInt(def);
	return p.first->AsNum();
}

ConfVarNumber* ConfVarTable::SetNum(const char *name, int value)
{
	ConfVarNumber *v = GetVar(name, ConfVar::typeNumber).first->AsNum();
	v->SetInt(value);
	return v;
}

ConfVarBool* ConfVarTable::GetBool(const char *name, bool def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeBoolean);
	if( !p.second )
		p.first->AsBool()->Set(def);
	return p.first->AsBool();
}

ConfVarBool* ConfVarTable::SetBool(const char *name, bool value)
{
	ConfVarBool *v = GetVar(name, ConfVar::typeBoolean).first->AsBool();
	v->Set(value);
	return v;
}

ConfVarString* ConfVarTable::GetStr(const char *name, const char* def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeString);
	if( !p.second )
		p.first->AsStr()->Set(def);
	return p.first->AsStr();
}

ConfVarString* ConfVarTable::SetStr(const char *name, const char* value)
{
	ConfVarString *v = GetVar(name, ConfVar::typeString).first->AsStr();
	v->Set(value);
	return v;
}

ConfVarArray* ConfVarTable::GetArray(const char *name)
{
	return GetVar(name, ConfVar::typeArray).first->AsArray();
}

ConfVarTable* ConfVarTable::GetTable(const char *name)
{
	return GetVar(name, ConfVar::typeTable).first->AsTable();
}

bool ConfVarTable::Remove(ConfVar * const value)
{
	_ASSERT( typeTable == _type );
	for( std::map<string_t, ConfVar*>::iterator it = _val.asTable->begin();
		_val.asTable->end() != it; ++it )
	{
		if( value == it->second )
		{
			delete it->second;
			_val.asTable->erase(it);
			return true;
		}
	}
	return false;
}

bool ConfVarTable::Remove(const char *name)
{
	_ASSERT( typeTable == _type );
	std::map<string_t, ConfVar*>::iterator it = _val.asTable->find(name);
	if( _val.asTable->end() != it )
	{
		_val.asTable->erase(it);
		return true;
	}
	return false;
}

bool ConfVarTable::_Save(FILE *file, int level) const
{
	if( level ) fprintf(file, "{\n");

	bool delim = false;
	for( std::map<string_t, ConfVar*>::const_iterator it = _val.asTable->begin();
	     _val.asTable->end() != it; ++it )
	{
		if( level )
		{
			if( delim ) fputs(",\n", file);
			for( int i = 0; i < level; ++i )
			{
				fputs("  ", file);
			}
		}
		delim = true;

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

		if( !it->second->_Save(file, level+1) )
			return false;

		if( !level ) fputs(";\n", file);
	}
	if( level )
	{
		fputs("\n", file);
		for( int i = 0; i < level-1; ++i )
		{
			fputs("  ", file);
		}
		fputs("}", file);
	}

	return true;
}

bool ConfVarTable::_Load(lua_State *L)
{
	// enumerate all fields of the table
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		// use only string keys
		if( LUA_TSTRING == lua_type(L, -2) )
		{
			const char *key = lua_tostring(L, -2);
			if( ConfVar *v = FromLuaType(L, this, key) )
			{
				if( !v->_Load(L) )
					return false;
			}
		}
		else
		{
			g_console->puts("WARNING: key value is not a string\n");
		}
	}

	return true;
}

bool ConfVarTable::Save(const char *filename) const
{
	FILE *file = fopen(filename, "w");
	fprintf(file, "-- config file\n-- don't modify!\n\n");
	bool result = _Save(file, 0);
	fclose(file);
	return result;
}

bool ConfVarTable::Load(const char *filename)
{
	lua_State *L = lua_open();

	// try to read and execute the file
	if( luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0) )
	{
		g_console->printf("%s\n", lua_tostring(L, -1));
		lua_close(L);
		return false;
	}

	// get global table
	lua_pushvalue(L, LUA_GLOBALSINDEX);
	bool result = _Load(L);

	lua_close(L);

	return result;
}

void ConfVarTable::Push(lua_State *L)
{
	*reinterpret_cast<ConfVarTable**>( lua_newuserdata(L, sizeof(this)) ) = this;
	luaL_getmetatable(L, "conf_table");  // metatable for config
	lua_setmetatable(L, -2);
}


///////////////////////////////////////////////////////////////////////////////
// lua interface implementation

// returns type string for arrays and tables
static int luaT_conftostring(lua_State *L)
{
	_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);
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
		_ASSERT(FALSE);
	}
	return 1;
}

// retrieving an array element
static int luaT_getconfarray(lua_State *L)
{
	_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);

	ConfVarArray *v = *reinterpret_cast<ConfVarArray **>( lua_touserdata(L, 1) );
	_ASSERT( ConfVar::typeArray == v->GetType() );

	if( lua_isnumber(L, 2) )
	{
		size_t index = lua_tointeger(L, 2);
		if( index >= v->AsArray()->GetSize() )
		{
			return luaL_error(L, "array index is out of range");
		}
		v->GetAt(index)->Push(L);
	}
	else
	{
		return luaL_error(L, "number expected");
	}

	return 1;
}

// retrieving a table element
static int luaT_getconftable(lua_State *L)
{
	_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>( lua_touserdata(L, 1) );
	_ASSERT( ConfVar::typeTable == v->GetType() );

	if( lua_isstring(L, 2) )
	{
		const char *key = lua_tostring(L, 2);
		if( ConfVar *result = v->Find(key) )
		{
			result->Push(L);
		}
		else
		{
			return luaL_error(L, "variable '%s' doesn't exists", key);
		}
	}
	else
	{
		return luaL_error(L, "string expected");
	}

	return 1;
}

// assinging a value to an array element
static int luaT_setconfarray(lua_State *L)
{
	_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);

	ConfVarArray *v = *reinterpret_cast<ConfVarArray **>( lua_touserdata(L, 1) );
	_ASSERT( ConfVar::typeArray == v->GetType() );

	if( lua_isnumber(L, 2) )
	{
		size_t index = lua_tointeger(L, 2);
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
			val->AsNum()->SetFloat( (float) lua_tonumber(L, 3) );
			break;
		case ConfVar::typeString:
			val->AsStr()->Set( lua_tostring(L, 3) );
			break;
		case ConfVar::typeArray:
			return luaL_error(L, "attempt to modify conf_array");
		case ConfVar::typeTable:
			return luaL_error(L, "attempt to modify conf_table");
		}
	}
	else
	{
		return luaL_error(L, "number expected");
	}

	return 0;
}

// assinging a value to a table element
static int luaT_setconftable(lua_State *L)
{
	_ASSERT(lua_type(L, 1) == LUA_TUSERDATA);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>( lua_touserdata(L, 1) );
	_ASSERT( ConfVar::typeTable == v->GetType() );

	if( lua_isstring(L, 2) )
	{
		const char *key = lua_tostring(L, 2);
		if( ConfVar *val = v->Find(key) )
		{
			switch( val->GetType() )
			{
			case ConfVar::typeBoolean:
				val->AsBool()->Set( 0 != lua_toboolean(L, 3) );
				break;
			case ConfVar::typeNumber:
				val->AsNum()->SetFloat( (float) lua_tonumber(L, 3) );
				break;
			case ConfVar::typeString:
				val->AsStr()->Set( lua_tostring(L, 3) );
				break;
			case ConfVar::typeArray:
				return luaL_error(L, "attempt to modify conf_array");
			case ConfVar::typeTable:
				return luaL_error(L, "attempt to modify conf_table");
			}
		}
		else
		{
			return luaL_error(L, "variable not found");
		}
	}
	else
	{
		return luaL_error(L, "string expected");
	}
	return 0;
}

// generic __next support for tables
static int luaT_conftablenext(lua_State *L)
{
	lua_settop(L, 2);
	luaL_checktype(L, 1, LUA_TUSERDATA);

	ConfVarTable *v = *reinterpret_cast<ConfVarTable **>( lua_touserdata(L, 1) );
	_ASSERT( ConfVar::typeTable == v->GetType() );

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
void InitConfigLuaBinding(lua_State *L, ConfVarTable *conf, const char *globName)
{
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

	conf->Push(L);
	lua_setglobal(L, globName);    // set global and pop one element from stack
}

///////////////////////////////////////////////////////////////////////////////
// end of file

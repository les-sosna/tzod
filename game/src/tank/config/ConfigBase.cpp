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
}

bool ConfVarString::_Save(FILE *file, int level) const
{
	fprintf(file, "\"%s\"", Get());
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
	_val.asArray  = new std::vector<ConfVar*>();
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
	}
	else
	if( newSize < oldSize )
	{
		for( size_t i = newSize; i < oldSize; ++i )
		{
			delete (*_val.asArray)[i];
		}
		_val.asArray->resize(newSize);
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

bool ConfVarTable::_Save(FILE *file, int level) const
{
	if( level ) fprintf(file, "{\n");

	bool delim = false;
	for( std::map<string_t, ConfVar*>::const_iterator it = _val.asTable->begin(); 
	     _val.asTable->end() != it; ++it )
	{
		if( level )
		{
			if( delim ) fprintf(file, ",\n");
			for(int i = 0; i < level; ++i )
			{
				fprintf(file, "  ");
			}
		}
		delim = true;
		fprintf(file, "%s = ", it->first.c_str());

		if( !it->second->_Save(file, level+1) )
			return false;

		if( !level ) fprintf(file, ";\n");
	}
	if( level ) fprintf(file, "\n}");

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
	if( 0 != (luaL_loadfile(L, filename) || lua_pcall(L, 0, 0, 0)) )
	{
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
// end of file

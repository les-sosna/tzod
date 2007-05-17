// ConfigBase.cpp

#include "stdafx.h"
#include "ConfigBase.h"

#include "core/Console.h"


///////////////////////////////////////////////////////////////////////////////

ConfVar::ConfVar()
{
	_type    = typeNil;
	_val.ptr = NULL;
}

ConfVar::~ConfVar()
{
	Free();
}

void ConfVar::SetType(Type type)
{
	Free();

	switch( type )
	{
	case typeNil:
		_val.ptr = NULL;
		break;
	case typeNumber:
		_val.asNumber = 0;
		break;
	case typeString:
		_val.asString = new string_t();
		break;
	case typeConfig:
		_val.asConfig = new Config();
		break;
	case typeArray:
		_val.asArray  = new std::vector<ConfVar*>();
		break;
	default:
		_ASSERT(FALSE);
	}

	_type = type;
}

void ConfVar::Free()
{
	switch( _type )
	{
	case typeNil:
		_ASSERT(NULL == _val.ptr);
		break;
	case typeNumber:
		break;
	case typeString:
		delete _val.asString;
		break;
	case typeConfig:
		delete _val.asConfig;
		break;
	case typeArray:
		delete _val.asArray;
		break;
	default:
		_ASSERT(FALSE);
	}

	_val.ptr = NULL;
	_type = typeNil;
}

ConfVarNumber* ConfVar::AsNum()
{
	_ASSERT(typeNumber == _type);
	return static_cast<ConfVarNumber*>(this);
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

ConfVarConfig* ConfVar::AsConf()
{
	_ASSERT(typeConfig == _type);
	return static_cast<ConfVarConfig*>(this);
}

///////////////////////////////////////////////////////////////////////////////


//
// scalar types access functions
//

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


//
// array access functions
//

void ConfVarArray::Resize(size_t newSize)
{
	_ASSERT(typeArray == _type);
	_val.asArray->resize(newSize);
}
size_t ConfVarArray::GetSize() const
{
	_ASSERT(typeArray == _type);
	return _val.asArray->size();
}
ConfVar* ConfVarArray::GetAt(size_t index)
{
	_ASSERT(typeArray == _type);
	return (*_val.asArray)[index];
}


//
// config access functions
//

Config* ConfVarConfig::Get() const
{
	_ASSERT(typeConfig == _type);
	return _val.asConfig;
}


///////////////////////////////////////////////////////////////////////////////

Config::Config()
{
}

Config::~Config()
{
	for( ValuesMap::iterator it = _values.begin(); _values.end() != it; ++it )
	{
		delete it->second;
	}
}

std::pair<ConfVar*, bool> Config::GetVar(const char *name, ConfVar::Type type)
{
	std::pair<ConfVar*, bool> result(NULL, true);

	_ASSERT( ConfVar::typeNil != type );
	ValuesMap::iterator it = _values.find(name);
	if( _values.end() == it )
	{
		result.first = new ConfVar();
		ValuePair pair(name, result.first);
		_values.insert(pair);
	}
	else
	{
		result.first = it->second;
	}

	if( result.first->GetType() != type )
	{
		if( result.first->GetType() != ConfVar::typeNil )
		{
			static const char* typeNames[] = {
				"nil", "float", "long", "string", "config", "array"
			};
			g_console->printf("WARNING: changing type of variable '%s' from %s to %s\n", 
				name, typeNames[result.first->GetType()], typeNames[type] );
		}

		result.first->SetType(type);
		result.second = false;
	}

	return result;
}

ConfVarNumber* Config::GetNum(const char *name, float def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetFloat(def);
	return p.first->AsNum();
}

ConfVarNumber* Config::SetNum(const char *name, float value)
{
	ConfVarNumber *v = GetVar(name, ConfVar::typeNumber).first->AsNum();
	v->SetFloat(value);
	return v;
}

ConfVarNumber* Config::GetNum(const char *name, int def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeNumber);
	if( !p.second )
		p.first->AsNum()->SetInt(def);
	return p.first->AsNum();
}

ConfVarNumber* Config::SetNum(const char *name, int value)
{
	ConfVarNumber *v = GetVar(name, ConfVar::typeNumber).first->AsNum();
	v->SetInt(value);
	return v;
}

ConfVarString* Config::GetStr(const char *name, const char* def)
{
	std::pair<ConfVar*, bool> p = GetVar(name, ConfVar::typeString);
	if( !p.second )
		p.first->AsStr()->Set(def);
	return p.first->AsStr();
}

ConfVarString* Config::SetStr(const char *name, const char* value)
{
	ConfVarString *v = GetVar(name, ConfVar::typeString).first->AsStr();
	v->Set(value);
	return v;
}

ConfVarConfig* Config::GetConf(const char *name)
{
	return GetVar(name, ConfVar::typeConfig).first->AsConf();
}

bool Config::_Save(FILE *file, int level) const
{
	bool delim = false;
	for( ValuesMap::const_iterator it = _values.begin(); _values.end() != it; ++it )
	{
		if( level )
		{
			if( delim )
			{
				fprintf(file, ",\n");
			}
			for(int i = 0; i < level; ++i )
			{
				fprintf(file, "  ");
			}
		}
		delim = true;
		fprintf(file, "%s = ", it->first.c_str());

		ConfVar* v = it->second;
		switch( v->GetType() )
		{
		case ConfVar::typeNil:
			break;
		case ConfVar::typeNumber:
			fprintf(file, "%g", v->AsNum()->GetFloat());
			break;
		case ConfVar::typeString:
			fprintf(file, "\"%s\"", v->AsStr()->Get());
			break;
		case ConfVar::typeConfig:
			fprintf(file, "{\n");
			if( !v->AsConf()->Get()->_Save(file, level + 1) )
				return false;
			fprintf(file, "\n}");
			break;
		case ConfVar::typeArray:
			fprintf(file, "{\n");
			for( size_t i = 0; i < v->AsArray()->GetSize(); ++i )
			{

			}
			fprintf(file, "\n}");
			break;
		default:
			_ASSERT(FALSE);
		}

		if( !level )
		{
			fprintf(file, ";\n");
		}
	}	
	return true;
}

bool Config::Save(const char *filename) const
{
	FILE *file = fopen(filename, "w");
	fprintf(file, "-- config file\n-- don't modify!\n\n");
	bool result = _Save(file, 0);
	fclose(file);
	return result;
}

bool Config::_Load(lua_State *L)
{
	// enumerate all fields of the table
	for( lua_pushnil(L); lua_next(L, -2); lua_pop(L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		// use only string keys
		if( LUA_TSTRING == lua_type(L, -2) )
		{
			const char *key = lua_tostring(L, -2);

			int valueType = lua_type(L, -1);
			switch( valueType )
			{
			case LUA_TSTRING:
				SetStr(key, lua_tostring(L, -1));
				break;
			case LUA_TNUMBER:
				SetNum(key, (float) lua_tonumber(L, -1));
				break;
			case LUA_TTABLE:
				// decide whether it array or subconfig
				if( lua_objlen(L, -1) )
				{
					// value is an array
					
				}
				else
				{
					// value is a subconfig
					GetConf(key)->Get()->_Load(L);
				}
				break;
			default:
				g_console->printf("WARNING: unknown value type - %s\n", lua_typename(L, -1));
				break;
			}
		}
		else
		{
			g_console->puts("WARNING: key value is not a string\n");
		}
	}

	return true;
}

bool Config::Load(const char *filename)
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
	int t = lua_type(L, -1);

	bool result = _Load(L);

	lua_close(L);

	return result;
}


///////////////////////////////////////////////////////////////////////////////
// end of file

// ConfigBase.h

#pragma once

// forward declarations
class Config;

class ConfVarNumber;
class ConfVarBool;
class ConfVarString;
class ConfVarArray;
class ConfVarConfig;

//
class ConfVar
{
public:
	enum Type
	{
		typeNil,
		typeNumber,
		typeBoolean,
		typeString,
		typeConfig,
		typeArray,
	};

	ConfVar();
	~ConfVar();

	void SetType(Type type); 
	Type GetType() const { return _type; }

	// type casting
	ConfVarNumber* AsNum();
	ConfVarBool*   AsBool();
	ConfVarString* AsStr();
	ConfVarArray*  AsArray();
	ConfVarConfig* AsConf();


protected:
	union Value
	{
		lua_Number              asNumber;
		bool                    asBool;
		string_t               *asString;
		std::vector<ConfVar*>  *asArray;
		Config                 *asConfig;
		void                   *ptr;
	};

	Type  _type;
	Value _val;

private:
	void Free();     // this also changes type to nil
};

class ConfVarNumber : public ConfVar
{
public:
	float GetFloat() const;
	void SetFloat(float value);

	int GetInt() const;
	void SetInt(int value);
};

class ConfVarBool : public ConfVar
{
public:
	bool Get() const;
	void Set(bool value);
};

class ConfVarString : public ConfVar
{
public:
	const char* Get() const;
	void Set(const char* value);
};

class ConfVarArray : public ConfVar
{
public:
	void      Resize(size_t newSize);
	size_t    GetSize() const;
	ConfVar*  GetAt(size_t index);
};

class ConfVarConfig : public ConfVar
{
public:
	Config* Get() const;
};


///////////////////////////////////////////////////////////////////////////////

class Config
{
	typedef std::map<string_t, ConfVar*> ValuesMap;
	typedef std::pair<string_t, ConfVar*> ValuePair;
	ValuesMap _values;

	bool _Save(FILE *file, int level) const;
	bool _Load(lua_State *L);
	
public:
	Config();
	~Config();

	// bool part contains true if value with sprecified type was found
	std::pair<ConfVar*, bool> GetVar(const char *name, ConfVar::Type type);

	ConfVarNumber* GetNum(const char *name, float def);
	ConfVarNumber* GetNum(const char *name, int  def);
	ConfVarBool*  GetBool(const char *name, bool def);
	ConfVarString* GetStr(const char *name, const char* def);


	ConfVarNumber* SetNum(const char *name, float value);
	ConfVarNumber* SetNum(const char *name, int  value);
	ConfVarBool*  SetBool(const char *name, bool value);
	ConfVarString* SetStr(const char *name, const char* value);

	ConfVarConfig* GetConf(const char *name);


	bool Save(const char *filename) const;
	bool Load(const char *filename);
};

///////////////////////////////////////////////////////////////////////////////
// end of file
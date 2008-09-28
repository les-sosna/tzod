// ConfigBase.h

#pragma once

// forward declarations
class ConfVarNumber;
class ConfVarBool;
class ConfVarString;
class ConfVarArray;
class ConfVarTable;

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
		typeArray,
		typeTable,
	};

	ConfVar();
	virtual ~ConfVar();

	void SetType(Type type);
	Type GetType() const { return _type; }
	virtual const char* GetTypeName() const;

	// type casting
	ConfVarNumber* AsNum();
	ConfVarBool*   AsBool();
	ConfVarString* AsStr();
	ConfVarArray*  AsArray();
	ConfVarTable*  AsTable();

	// lua binding
	virtual void Push(lua_State *L);

	Delegate<void(void)> eventChange;

protected:
	virtual bool _Save(FILE *file, int level) const;
	virtual bool _Load(lua_State *L);

	// to call _Load and _Save
	friend class ConfVarArray;
	friend class ConfVarTable;

public:
	union Value
	{
		lua_Number                     asNumber;
		bool                           asBool;
		string_t                      *asString;
		std::deque<ConfVar*>          *asArray;
		std::map<string_t, ConfVar*>  *asTable;
		void                          *ptr;
	};

	Type  _type;
	Value _val;
};

class ConfVarNumber : public ConfVar
{
public:
	ConfVarNumber();

	virtual const char* GetTypeName() const;

	float GetFloat() const;
	void SetFloat(float value);

	int GetInt() const;
	void SetInt(int value);

	void Push(lua_State *L);

protected:
	virtual bool _Save(FILE *file, int level) const;
	virtual bool _Load(lua_State *L);
};

class ConfVarBool : public ConfVar
{
public:
	ConfVarBool();

	virtual const char* GetTypeName() const;

	bool Get() const;
	void Set(bool value);

	void Push(lua_State *L);

protected:
	virtual bool _Save(FILE *file, int level) const;
	virtual bool _Load(lua_State *L);
};

class ConfVarString : public ConfVar
{
public:
	ConfVarString();
	virtual ~ConfVarString();

	virtual const char* GetTypeName() const;

	const string_t& Get() const;
	void Set(const string_t &value);

	void Push(lua_State *L);

protected:
	virtual bool _Save(FILE *file, int level) const;
	virtual bool _Load(lua_State *L);
};

class ConfVarArray : public ConfVar
{
public:
	ConfVarArray();
	virtual ~ConfVarArray();

	virtual const char* GetTypeName() const;

	// bool part contains true if value with the specified type was found
	std::pair<ConfVar*, bool> GetVar(size_t index, ConfVar::Type type);

	ConfVarNumber* GetNum(size_t index, float def);
	ConfVarNumber* GetNum(size_t index, int   def = 0);
	ConfVarBool*  GetBool(size_t index, bool  def = false);
	ConfVarString* GetStr(size_t index, const char* def = "");

	ConfVarNumber* SetNum(size_t index, float value);
	ConfVarNumber* SetNum(size_t index, int   value);
	ConfVarBool*  SetBool(size_t index, bool  value);
	ConfVarString* SetStr(size_t index, const char* value);

	ConfVarArray* GetArray(size_t index);
	ConfVarTable* GetTable(size_t index);


	void      Resize(size_t newSize);
	size_t    GetSize() const;
	
	ConfVar*  GetAt(size_t index) const;
	void      RemoveAt(size_t index);

	void      PopFront();
	void      PopBack();
	ConfVar*  PushFront(Type type);
	ConfVar*  PushBack(Type type);

	void Push(lua_State *L);

protected:
	bool _Save(FILE *file, int level) const;
	bool _Load(lua_State *L);
};

class ConfVarTable : public ConfVar
{
public:
	ConfVarTable();
	virtual ~ConfVarTable();

	virtual const char* GetTypeName() const;

	ConfVar* Find(const string_t &name); // returns NULL if variable not found
	size_t GetSize() const;
	void GetKeyList(std::vector<string_t> &out) const;

	// bool part contains true if value with the specified type was found
	std::pair<ConfVar*, bool> GetVar(const string_t &name, ConfVar::Type type);

	ConfVarNumber* GetNum(const string_t &name, float def);
	ConfVarNumber* GetNum(const string_t &name, int   def = 0);
	ConfVarBool*  GetBool(const string_t &name, bool  def = false);
	ConfVarString* GetStr(const string_t &name, const char* def = "");

	ConfVarNumber* SetNum(const string_t &name, float value);
	ConfVarNumber* SetNum(const string_t &name, int   value);
	ConfVarBool*  SetBool(const string_t &name, bool  value);
	ConfVarString* SetStr(const string_t &name, const string_t &value);

	ConfVarArray* GetArray(const string_t &name);
	ConfVarTable* GetTable(const string_t &name);

	bool Remove(ConfVar * const value);
	bool Remove(const string_t &name);
	bool Rename(ConfVar * const value, const string_t &newName);
	bool Rename(const string_t &oldName, const string_t &newName);

	bool Save(const char *filename) const;
	bool Load(const char *filename);

	void Push(lua_State *L);

protected:
	bool _Save(FILE *file, int level) const;
	bool _Load(lua_State *L);
};

///////////////////////////////////////////////////////////////////////////////
// config cache

class LuaConfigCacheBase
{
	class helper
	{
		ConfVarTable *_root;
	public:
		void InitLuaBinding(lua_State *L, const char *name);
	};
	helper _helper;

public:
	LuaConfigCacheBase();
	virtual ~LuaConfigCacheBase();
	helper* operator -> ();
};

///////////////////////////////////////////////////////////////////////////////
// Lua binding

void InitConfigLuaBinding(lua_State *L, ConfVarTable *conf, const char *globName);


///////////////////////////////////////////////////////////////////////////////
// helper macros



///////////////////////////////////////////////////////////////////////////////
// end of file
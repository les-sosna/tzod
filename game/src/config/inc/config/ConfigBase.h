#pragma once
#include <string>
#include <functional>
#include <map>
#include <memory>
#include <deque>
#include <vector>

class ConfVarNumber;
class ConfVarBool;
class ConfVarString;
class ConfVarArray;
class ConfVarTable;
struct lua_State;

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

	void SetHelpString(std::string str) { _help = std::move(str); }
	const std::string& GetHelpString() const { return _help; }

	void SetType(Type type);
	Type GetType() const { return _type; }
	virtual const char* GetTypeName() const;

	// type casting
	ConfVarNumber& AsNum();
	ConfVarBool&   AsBool();
	ConfVarString& AsStr();
	ConfVarArray&  AsArray();
	ConfVarTable&  AsTable();

	// lua binding helpers
	void Freeze(bool freeze);
	bool IsFrozen() const { return _frozen; }
	virtual void Push(lua_State *L) const;
	virtual bool Assign(lua_State *L);

	// notifications
	std::function<void(void)> eventChange;

	// serialization
	virtual bool Write(FILE *file, int indent) const;

protected:
	void FireValueUpdate(ConfVar *pVar);

	typedef std::map<std::string, std::unique_ptr<ConfVar>> TableType;

	union Value
	{
		double                         asNumber;
		bool                           asBool;
		std::string                   *asString;
		std::deque<std::unique_ptr<ConfVar>> *asArray;
		TableType                     *asTable;
	};

	Type  _type;
	Value _val;
	std::string _help;
	bool _frozen;    // frozen value can not change its type and also its content in case of table
};

class ConfVarNumber : public ConfVar
{
public:
	ConfVarNumber();
	virtual ~ConfVarNumber();


	double GetRawNumber() const;
	void SetRawNumber(double value);

	float GetFloat() const;
	void SetFloat(float value);

	int GetInt() const;
	void SetInt(int value);

	// ConfVar
	virtual const char* GetTypeName() const;
	virtual void Push(lua_State *L) const;
	virtual bool Assign(lua_State *L);
	virtual bool Write(FILE *file, int indent) const;
};

class ConfVarBool : public ConfVar
{
public:
	ConfVarBool();
	virtual ~ConfVarBool();

	bool Get() const;
	void Set(bool value);

	// ConfVar
	virtual const char* GetTypeName() const;
	virtual void Push(lua_State *L) const;
	virtual bool Assign(lua_State *L);
	virtual bool Write(FILE *file, int indent) const;
};

class ConfVarString : public ConfVar
{
public:
	ConfVarString();
	virtual ~ConfVarString();


	const std::string& Get() const;
	void Set(std::string value);

	// ConfVar
	virtual const char* GetTypeName() const;
	virtual void Push(lua_State *L) const;
	virtual bool Assign(lua_State *L);
	virtual bool Write(FILE *file, int indent) const;
};

class ConfVarArray : public ConfVar
{
public:
	ConfVarArray();
	virtual ~ConfVarArray();

	// bool part contains true if value with the specified type was found
	std::pair<ConfVar*, bool> GetVar(size_t index, ConfVar::Type type);

	ConfVarNumber& GetNum(size_t index, float def);
	ConfVarNumber& GetNum(size_t index, int   def = 0);
	ConfVarBool&  GetBool(size_t index, bool  def = false);
	ConfVarString& GetStr(size_t index);
	ConfVarString& GetStr(size_t index, std::string def);

	ConfVarNumber& SetNum(size_t index, float value);
	ConfVarNumber& SetNum(size_t index, int   value);
	ConfVarBool&  SetBool(size_t index, bool  value);
	ConfVarString& SetStr(size_t index, std::string value);

	ConfVarArray& GetArray(size_t index);
	ConfVarTable& GetTable(size_t index);

	void      EnsureIndex(size_t index);
	void      Resize(size_t newSize);
	size_t    GetSize() const;

	ConfVar&  GetAt(size_t index) const;
	void      RemoveAt(size_t index);

	void      PopFront();
	void      PopBack();
	ConfVar&  PushFront(Type type);
	ConfVar&  PushBack(Type type);

	// ConfVar
	virtual const char* GetTypeName() const;
	virtual void Push(lua_State *L) const;
	virtual bool Assign(lua_State *L);
	virtual bool Write(FILE *file, int indent) const;
};

class ConfVarTable : public ConfVar
{
public:
	ConfVarTable();
	virtual ~ConfVarTable();

	ConfVar* Find(const std::string &name); // returns nullptr if variable not found
	size_t GetSize() const;

	typedef std::vector<std::string> KeyListType;
	KeyListType GetKeys() const;

	// bool part contains true if value with the specified type was found
	std::pair<ConfVar*, bool> GetVar(const std::string &name, ConfVar::Type type);

	ConfVarNumber& GetNum(std::string name, float def);
	ConfVarNumber& GetNum(std::string name, int   def = 0);
	ConfVarBool&  GetBool(std::string name, bool  def = false);
	ConfVarString& GetStr(std::string name);
	ConfVarString& GetStr(std::string name, std::string def);

	ConfVarNumber& SetNum(std::string name, float value);
	ConfVarNumber& SetNum(std::string name, int   value);
	ConfVarBool&  SetBool(std::string name, bool  value);
	ConfVarString& SetStr(std::string name, std::string value);

	ConfVarArray& GetArray(std::string name, void (*init)(ConfVarArray&) = nullptr);
	ConfVarTable& GetTable(std::string name, void (*init)(ConfVarTable&) = nullptr);

	void Clear();
	bool Remove(const ConfVar &value);
	bool Remove(const std::string &name);
	bool Rename(const ConfVar &value, std::string newName);
	bool Rename(const std::string &oldName, std::string newName);

	bool Save(const char *filename) const;
	bool Load(const char *filename);

	// Lua binding
	void InitConfigLuaBinding(lua_State *L, const char *globName);

	// ConfVar
	virtual const char* GetTypeName() const;
	virtual void Push(lua_State *L) const;
	virtual bool Assign(lua_State *L);
	virtual bool Write(FILE *file, int indent) const;

protected:
	static int luaT_conftablenext(lua_State *L);
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

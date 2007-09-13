// Object.h

#pragma once

#include "notify.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations
class MapFile;
class SaveFile;

class GC_Object;

///////////////////////////////////////////////////////////////////////////////

typedef PtrList<GC_Object> OBJECT_LIST;
typedef GridSet<OBJECT_LIST> OBJECT_GRIDSET;

/////////////////////////////////////////
// rtti and serialization

#define DECLARE_SELF_REGISTRATION(cls)  \
private:                                \
typedef cls __ThisClass;                \
static ObjectType __thisType;           \
static bool __registered;               \
static bool __SelfRegister();           \
public:                                 \
static ObjectType GetTypeStatic()       \
{                                       \
	return __thisType;                  \
}                                       \
virtual ObjectType GetType()            \
{                                       \
	return __thisType;                  \
}                                       \
private:


#define IMPLEMENT_SELF_REGISTRATION(cls)                               \
ObjectType cls::__thisType = __RegisterType<cls>(typeid(cls).name());  \
bool cls::__registered = cls::__SelfRegister();                        \
bool cls::__SelfRegister()


// for template classes (experimental)
#define IMPLEMENT_SELF_REGISTRATION_T(cls)             \
template<class T>                                      \
ObjectType cls<T>::__thisType = (cls<T>::__registered, \
	__RegisterType<cls<T> >(typeid(cls<T>).name()));   \
template<class T>                                      \
bool cls<T>::__registered = cls<T>::__SelfRegister();  \
template<class T>                                      \
bool cls<T>::__SelfRegister()


///////////////////////////////////////////////////////////

class ObjectProperty
{
public:
	enum PropertyType
	{
		TYPE_INTEGER,
		TYPE_STRING,
		TYPE_MULTISTRING,
	};

private:
	string_t               _name;
	PropertyType           _type;
	int                    _int_value;
	int                    _int_min;
	int                    _int_max;
	string_t               _str_value;
	std::vector<string_t>  _value_set;
	size_t                 _value_index;

public:
	ObjectProperty(PropertyType type, const string_t &name);

	const string_t& GetName(void) const;
	PropertyType GetType(void) const;


	//
	// TYPE_INTEGER
	//
	int  GetValueInt(void) const;
	int  GetMin(void) const;
	int  GetMax(void) const;
	void SetValueInt(int value);
	void SetRange(int min, int max);


	//
	// TYPE_STRING
	//
	void SetValue(const string_t &str);
	const string_t& GetValue(void) const;


	//
	// TYPE_MULTISTRING
	//
	void   AddItem(const string_t &str);
	size_t GetCurrentIndex(void) const;
	void   SetCurrentIndex(size_t index);
	size_t GetSetSize(void) const;
	const string_t& GetSetValue(size_t index) const;
};

class PropertySet : public RefCounted
{
	GC_Object       *_object;
	ObjectProperty   _propName;

protected:
	GC_Object* GetObject() const;

public:
	PropertySet(GC_Object *object);

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
	virtual void Exchange(bool applyToObject);
};

////////////////////////////////////////////////////////////
// object flags

// general
#define GC_FLAG_OBJECT_KILLED                 0x00000001
#define GC_FLAG_OBJECT_NAMED                  0x00000002

// engine events
#define GC_FLAG_OBJECT_EVENTS_TS_FIXED        0x00000004
#define GC_FLAG_OBJECT_EVENTS_TS_FLOATING     0x00000008
#define GC_FLAG_OBJECT_EVENTS_ENDFRAME        0x00000010

#define GC_FLAG_OBJECT_EVENTS_ALL           \
	(GC_FLAG_OBJECT_EVENTS_TS_FIXED|        \
	GC_FLAG_OBJECT_EVENTS_TS_FLOATING|      \
	GC_FLAG_OBJECT_EVENTS_ENDFRAME)

#define GC_FLAG_OBJECT_                       0x00000020


enum GlobalListID
{
	LIST_objects,
	LIST_services,
	LIST_respawns,
	LIST_projectiles,
	LIST_players,
	LIST_sounds,
	LIST_indicators,
	LIST_vehicles,
	LIST_pickups,
	LIST_lights,
	LIST_cameras,
	//------------------
	GLOBAL_LIST_COUNT
};


typedef void (GC_Object::*NOTIFYPROC) (GC_Object *sender, void *param);

class GC_Object
{
protected:
	template<GlobalListID listId>
	class MemberOfGlobalList
	{
		OBJECT_LIST::iterator  _pos;
	public:
		MemberOfGlobalList(GC_Object *obj)
		{
			g_level->GetList(listId).push_front(obj);
			_pos = g_level->GetList(listId).begin();
		}
		~MemberOfGlobalList()
		{
			g_level->GetList(listId).safe_erase(_pos);
		}
	};

private:
	MemberOfGlobalList<LIST_objects> _memberOf;

	struct Notify
	{
		NotyfyType           type;
		bool                 once;      // событие должно быть удалено после исполнения
		bool                 removed;   // событие помечено для удаления
		bool                 hasGuard;  // у события есть пара OnKillSubscriber
		SafePtr<GC_Object>   subscriber;
		NOTIFYPROC           handler;
		//---------------------------------------
		inline Notify()  { removed = false; }
		inline ~Notify() { subscriber = NULL; }
		bool operator == (const Notify &src) const
		{
			return subscriber == src.subscriber && type == src.type && handler == src.handler;
		}
		struct CleanUp
		{
			bool operator() ( Notify &test ) { return test.removed; }
		};
		void Serialize(SaveFile &f);
	};


	//
	// attributes
	//

private:
	DWORD           _flags;             // некоторые свойства определяются флагами
	int             _refCount;          // число ссылок на объект. при создании = 1

	OBJECT_LIST::iterator _itPosFixed;      // позиция в Level::ts_fixed
	OBJECT_LIST::iterator _itPosFloating;   // позиция в Level::ts_floating
	OBJECT_LIST::iterator _itPosEndFrame;   // позиция в Level::endframe

	std::list<Notify> _notifyList;          // извещения, рассылаемые данным объектом
	int  _notifyProtectCount;               // счетчик блокировки удаления из списка _notifyList

protected:
	void SetFlags(DWORD flags)
	{
		_flags |= flags;
	}
	DWORD GetFlags()
	{
		return _flags;
	}
	void ClearFlags(DWORD flags)
	{
		_flags &= ~flags;
	}

	// return true if one of the flags is set
	bool CheckFlags(DWORD flags) const
	{
		return 0 != (_flags & flags);
	}


	//
	// access functions
	//

public:
	bool  IsKilled() const { return CheckFlags(GC_FLAG_OBJECT_KILLED); }


	//
	// construction/destruction
	//

	GC_Object();
	virtual ~GC_Object();


	//
	// operations
	//

private:
	void OnKillSubscriber(GC_Object *sender, void *param);

protected:
	void PulseNotify(NotyfyType type, void *param = NULL);

public:
	int  AddRef();
	int  Release();

	void SetEvents(DWORD dwEvents);

	const char* GetName() const;
	void SetName(const char *name);

	// использовать флаг guard=false можно только в том случае,
	// если подписчик гарантировано живет дольше, чем источник события
	void Subscribe(NotyfyType type, GC_Object *subscriber,
		NOTIFYPROC handler, bool once = true, bool guard = true);
	void Unsubscribe(GC_Object *subscriber);
	bool IsSubscriber(const GC_Object *object) const;
	GC_Object* GetSubscriber(ObjectType type)  const;


	//
	// serialization
	//

public:
	static GC_Object* CreateFromFile(SaveFile &file);
	virtual bool IsSaved() { return false; }
	virtual void Serialize(SaveFile &f);

protected:
	struct FromFile {};
	GC_Object(FromFile);

private:
	typedef GC_Object* (*LPFROMFILEPROC) (void);
	typedef std::map<ObjectType, LPFROMFILEPROC> __FromFileMap;
	static __FromFileMap& __GetFromFileMap()
	{
		static __FromFileMap ffm;
		return ffm;
	}
	template<class T> static GC_Object* __FromFileProc(void)
	{
		return new T(FromFile());
	}
    template<class T> static void __RegisterForSerialization(ObjectType type)
	{
		_ASSERT(__GetFromFileMap().end() == __GetFromFileMap().find(type));
		LPFROMFILEPROC pf = __FromFileProc<T>;
        __GetFromFileMap()[type] = pf;
	}


	//
	// RTTI support
	//

private:
	typedef std::map<string_t, size_t> __TypeMap;
	static __TypeMap& __GetTypeMap()
	{
		static __TypeMap tm;
		return tm;
	}

protected:
	template<class T>
	static ObjectType __RegisterType(const char *name)
	{
		size_t index = __GetTypeMap().size();
		__GetTypeMap()[name] = index;
		__RegisterForSerialization<T>((ObjectType) index);
		return (ObjectType) index;
	}

public:
	virtual ObjectType GetType() = 0;


	//
	// properties
	//
protected:
	typedef PropertySet MyPropertySet;

public:
	virtual SafePtr<PropertySet> GetProperties();

	//
	// overrides
	//

public:
	virtual void Kill();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
	virtual void EndFrame();
	virtual void EditorAction();

	virtual void mapExchange(MapFile &f);


	//
	// debug helpers
	//

#ifdef NETWORK_DEBUG
public:
	virtual DWORD checksum(void) const
	{
		return 0;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////
// end of file

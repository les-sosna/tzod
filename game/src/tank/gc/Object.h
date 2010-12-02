// Object.h

#pragma once

#include "notify.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations
class MapFile;
class SaveFile;

class GC_Object;

///////////////////////////////////////////////////////////////////////////////

typedef PtrList<GC_Object> ObjectList;

/////////////////////////////////////////
// memory management

#define DECLARE_POOLED_ALLOCATION(cls)          \
private:                                        \
    static MemoryManager<cls> __memoryManager;  \
public:                                         \
    void* operator new(size_t count)            \
    {                                           \
        assert(sizeof(cls) == count);          \
        return __memoryManager.Alloc();         \
    }                                           \
    void operator delete(void *ptr)             \
    {                                           \
        __memoryManager.Free((cls*) ptr);       \
    }

#define IMPLEMENT_POOLED_ALLOCATION(cls)        \
    MemoryManager<cls> cls::__memoryManager;



/////////////////////////////////////////
// rtti and serialization

#define DECLARE_SELF_REGISTRATION(cls)          \
    DECLARE_POOLED_ALLOCATION(cls)              \
    private:                                    \
        typedef cls __ThisClass;                \
        static ObjectType __thisType;           \
        static bool __registered;               \
        static bool __SelfRegister();           \
    public:                                     \
        static ObjectType GetTypeStatic()       \
        {                                       \
            return __thisType;                  \
        }                                       \
        virtual ObjectType GetType()            \
        {                                       \
            return __thisType;                  \
        }                                       \
    private:


#define IMPLEMENT_SELF_REGISTRATION(cls)                      \
    IMPLEMENT_POOLED_ALLOCATION(cls)                          \
    ObjectType cls::__thisType = __RegisterType<cls>(#cls);   \
    bool cls::__registered = cls::__SelfRegister();           \
    bool cls::__SelfRegister()


// for template classes (experimental)
#define IMPLEMENT_SELF_REGISTRATION_T(cls)                 \
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
		TYPE_FLOAT,
		TYPE_STRING,
		TYPE_MULTISTRING,
	};

private:
	string_t               _name;
	PropertyType           _type;
	string_t               _str_value;
	std::vector<string_t>  _value_set;
	union {
		size_t             _value_index;
		int                _int_value;
		float              _float_value;
	};
	union {
		int                _int_min;
		float              _float_min;
	};
	union {
		int                _int_max;
		float              _float_max;
	};

public:
	ObjectProperty(PropertyType type, const string_t &name);

	const string_t& GetName(void) const;
	PropertyType GetType(void) const;


	//
	// TYPE_INTEGER
	//
	int  GetIntValue(void) const;
	int  GetIntMin(void) const;
	int  GetIntMax(void) const;
	void SetIntValue(int value);
	void SetIntRange(int min, int max);

	//
	// TYPE_FLOAT
	//
	float GetFloatValue(void) const;
	float GetFloatMin(void) const;
	float GetFloatMax(void) const;
	void  SetFloatValue(float value);
	void  SetFloatRange(float min, float max);


	//
	// TYPE_STRING
	//
	void SetStringValue(const string_t &str);
	const string_t& GetStringValue(void) const;


	//
	// TYPE_MULTISTRING
	//
	void   AddItem(const string_t &str);
	size_t GetCurrentIndex(void) const;
	void   SetCurrentIndex(size_t index);
	size_t GetListSize(void) const;
	const string_t& GetListValue(size_t index) const;
};

class PropertySet : public RefCounted
{
	GC_Object       *_object;
	ObjectProperty   _propName;

protected:
	GC_Object* GetObject() const;
	virtual void MyExchange(bool applyToObject);

public:
	PropertySet(GC_Object *object);

	const char* GetTypeName() const;
	void LoadFromConfig();
	void SaveToConfig();
	void Exchange(bool applyToObject);

	Delegate<void(bool)> eventExchange;

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
};

////////////////////////////////////////////////////////////
// object flags

// general
#define GC_FLAG_OBJECT_KILLED                 0x00000001 // FIXME: this flag should not be serialized
#define GC_FLAG_OBJECT_NAMED                  0x00000002

// engine events
#define GC_FLAG_OBJECT_EVENTS_TS_FIXED        0x00000004

#define GC_FLAG_OBJECT_                       0x00000008


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
	GC_Object(const GC_Object&); // no copy
	GC_Object& operator = (const GC_Object&);

protected:
	// works if v is EXACTLY a power of 2
	static inline unsigned long FastLog2(unsigned long v)
	{
		static const unsigned long MultiplyDeBruijnBitPosition[32] =
		{
			0, 1, 28, 2, 29, 14, 24, 3, 30, 22, 20, 15, 25, 17, 4, 8,
			31, 27, 13, 23, 21, 19, 16, 7, 26, 12, 18, 6, 11, 5, 10, 9
		};
		assert(v == (1 << MultiplyDeBruijnBitPosition[(v * 0x077CB531U) >> 27]));
		return MultiplyDeBruijnBitPosition[(v * 0x077CB531U) >> 27];
	}

	template<GlobalListID listId>
	class MemberOfGlobalList
	{
		ObjectList::iterator  _pos;
	public:
		MemberOfGlobalList(GC_Object *obj)
		{
			g_level->GetList(listId).push_back(obj);
			_pos = g_level->GetList(listId).rbegin();
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
		DECLARE_POOLED_ALLOCATION(Notify);

		Notify *next;
		SafePtr<GC_Object>   subscriber;
		NOTIFYPROC           handler;
		NotifyType           type;
		bool IsRemoved() const
		{
			return !subscriber || subscriber->IsKilled();
		}
		void Serialize(SaveFile &f);
		explicit Notify(Notify *nxt) : next(nxt) {}
	};


	//
	// attributes
	//

private:
	DWORD           _flags;             // define various object properties
	int             _refCount;          // = 1 at the construction time

	ObjectList::iterator _itPosFixed;      // position in the Level::ts_fixed

	Notify *_firstNotify;
	int  _notifyProtectCount;

public:
	void SetFlags(DWORD flags, bool value)
	{
		_flags = value ? (_flags|flags) : (_flags & ~flags);
	}
	DWORD GetFlags() const
	{
		return _flags;
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
	bool IsKilled() const { return CheckFlags(GC_FLAG_OBJECT_KILLED); }



	//
	// construction/destruction
	//

	GC_Object();
	virtual ~GC_Object();


	//
	// operations
	//

protected:
	void PulseNotify(NotifyType type, void *param = NULL);

public:
	int  AddRef();
	int  Release();

	void SetEvents(DWORD dwEvents);

	const char* GetName() const;
	void SetName(const char *name);

	void Subscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler);
	void Unsubscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler);


	//
	// serialization
	//

public:
	static GC_Object* Create(ObjectType type);
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
		assert(__GetFromFileMap().end() == __GetFromFileMap().find(type));
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
	virtual PropertySet* NewPropertySet();

public:
	SafePtr<PropertySet> GetProperties();

	//
	// overrides
	//

public:
	virtual void Kill();

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
	virtual void EditorAction();

	virtual void MapExchange(MapFile &f);


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

// Object.h

#pragma once

#include "notify.h"

///////////////////////////////////////////////////////////////////////////////

class MapFile;
class SaveFile;

class GC_Object;

///////////////////////////////////////////////////////////////////////////////

typedef PtrList<GC_Object> OBJECT_LIST;
typedef GridSet<OBJECT_LIST> OBJECT_GRIDSET;

template <class T>
struct ObjectContext
{
	GridSet<T> *pGridSet;
	//-------
	typename T::iterator iterator;
	BOOL inContext;
};

/////////////////////////////////////////
// rtti and serialization

#define DECLARE_SELF_REGISTRATION(cls)	\
private:								\
typedef cls __this_class;				\
static bool _registered;				\
static bool _self_register();			\
public:									\
static ObjectType this_type;			\
virtual ObjectType GetType()			\
{										\
	return this_type;					\
} private:


#define IMPLEMENT_SELF_REGISTRATION(cls)								\
ObjectType cls::this_type = _register_type<cls>(typeid(cls).name());	\
bool cls::_registered = cls::_self_register();							\
bool cls::_self_register()


// for template classes (experimental)
#define IMPLEMENT_SELF_REGISTRATION_T(cls)				\
template<class T>										\
ObjectType cls<T>::this_type = (cls<T>::_registered,	\
	_register_type<cls<T> >(typeid(cls<T>).name()));	\
template<class T>										\
bool cls<T>::_registered = cls<T>::_self_register();	\
template<class T>										\
bool cls<T>::_self_register()


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

	string_t  GetName(void) const;
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
	string_t GetValue(void) const;


	//
	// TYPE_MULTISTRING
	//
	void    AddItem(const string_t &str);
	size_t  GetCurrentIndex(void) const;
	void    SetCurrentIndex(size_t index);
	size_t  GetSetSize(void) const;
	string_t GetSetValue(size_t index) const;
};

class IPropertySet
{
	SafePtr<GC_Object> _object;
	int _refcount;

protected:
	IPropertySet(GC_Object *object);
	virtual ~IPropertySet();

	template<class T> T* obj(void)
	{
		_ASSERT(dynamic_cast<T*>(GetRawPtr(_object)));
		return static_cast<T*>(GetRawPtr(_object));
	}

public:
	int AddRef();
	int Release();

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
	virtual void Exchange(bool bApply);
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


class GC_Object
{
	//
	// types
	//

protected:

	typedef ObjectContext<OBJECT_LIST> ObjectContext;
	typedef void (GC_Object::*NOTIFYPROC) (GC_Object *sender, void *param);

	class MemberOfGlobalList
	{
		OBJECT_LIST           *_list;
		OBJECT_LIST::iterator  _pos;
	public:
		MemberOfGlobalList(OBJECT_LIST &list, GC_Object *obj)
		{
			list.push_front(obj);
			_list = &list;
			_pos  = list.begin();
		}
		~MemberOfGlobalList()
		{
			_list->safe_erase(_pos);
		}
	};


private:
	typedef std::list<ObjectContext>::iterator CONTEXTS_ITERATOR;

	MemberOfGlobalList _memberOf;

	struct Notify
	{
		NotyfyType           type;
		bool                 once;		// событие должно быть удалено после исполнени€
		bool                 removed;	// событие помечено дл€ удалени€
		bool                 hasGuard;	// у событи€ есть пара OnKillSubscriber
		SafePtr<GC_Object>   subscriber;
		NOTIFYPROC           handler;
		//---------------------------------------
		inline Notify(const Notify &src)
		{
			type       = src.type;
			once       = src.once;
			removed    = src.removed;
			hasGuard   = src.hasGuard;
			subscriber = src.subscriber;
			handler    = src.handler;
		}
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
	DWORD           _flags;             // некоторые свойства определ€ютс€ флагами
	int             _refCount;          // число ссылок на объект. при создании = 1

	std::list<ObjectContext> _contexts;    // список контекстов данного объекта
	Location                 _location;    // координаты в контексте.

	OBJECT_LIST::iterator _itPosFixed;      // позици€ в Level::ts_fixed
	OBJECT_LIST::iterator _itPosFloating;   // позици€ в Level::ts_floating
	OBJECT_LIST::iterator _itPosEndFrame;   // позици€ в Level::endframe

	std::list<Notify> _notifyList;          // извещени€, рассылаемые данным объектом
	int  _notifyProtectCount;               // счетчик блокировки удалени€ из списка _notifyList

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


public:		// FIXME!
	vec2d			_pos;				// положение центра объекта в мире


	//
	// access functions
	//

public:
	int   GetLocationX()      const { return _location.x;     }
	int   GetLocationY()      const { return _location.y;     }
	int   GetLocationLevel()  const { return _location.level; }
	bool  IsKilled()          const { return CheckFlags(GC_FLAG_OBJECT_KILLED); }


	//
	// construction/destruction
	//

	GC_Object();
	virtual ~GC_Object();


	//
	// operations
	//

private:
	void LeaveAllContexts();
	void EnterAllContexts(const Location &l);
	void EnterContext(ObjectContext &context, const Location &l);
	void LeaveContext(ObjectContext &context);

	void OnKillSubscriber(GC_Object *sender, void *param);

protected:
	void PulseNotify(NotyfyType type, void *param = NULL);

public:
	static void LocationFromPoint(const vec2d &pt, Location &l);
	int  AddRef();
	int  Release();

	void AddContext(OBJECT_GRIDSET *pGridSet);
	void RemoveContext(OBJECT_GRIDSET *pGridSet);
	void SetEvents(DWORD dwEvents);

	const char* GetName() const;
	void SetName(const char *name);

	// использовать флаг guard=false можно только в том случае,
	// если подписчик гарантировано живет дольше, чем источник событи€
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
	typedef std::map<ObjectType, LPFROMFILEPROC> _from_file_map;
	static _from_file_map& _get_from_file_map()
	{
		static _from_file_map ffm;
		return ffm;
	}
	template<class T> static GC_Object* _from_file_proc(void)
	{
		return new T(FromFile());
	}
    template<class T> static void _register_for_serialization(ObjectType type)
	{
		_ASSERT(_get_from_file_map().end() == _get_from_file_map().find(type));
		LPFROMFILEPROC pf = _from_file_proc<T>;
        _get_from_file_map()[type] = pf;
	}


	//
	// rtti support
	//

private:
	typedef std::map<string_t, size_t> _type_map;
	static _type_map& _get_type_map()
	{
		static _type_map tm;
		return tm;
	}

protected:
	template<class T>
	static ObjectType _register_type(const char *name)
	{
		size_t index = _get_type_map().size();
		_get_type_map()[name] = index;
		_register_for_serialization<T>((ObjectType) index);
		return (ObjectType) index;
	}

public:
	virtual ObjectType GetType() = 0;



	//
	// overrides
	//

public:
	virtual void Kill();
	virtual void MoveTo(const vec2d &pos);

	virtual void TimeStepFixed(float dt);
	virtual void TimeStepFloat(float dt);
	virtual void EndFrame();
	virtual void EditorAction();

	virtual IPropertySet* GetProperties();
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

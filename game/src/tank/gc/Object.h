// Object.h

#pragma once

#include "GlobalListHelper.h"
#include "notify.h"
#include "ObjPtr.h"
#include "ObjectProperty.h"
#include "TypeSystem.h"
#include "core/Delegate.h"
#include "core/SafePtr.h"


class MapFile;
class SaveFile;

class GC_Object;
class World;

typedef PtrList<GC_Object> ObjectList;

/////////////////////////////////////////
// memory management

#define DECLARE_POOLED_ALLOCATION(cls)          \
private:                                        \
    static MemoryPool<cls, sizeof(int)> __pool; \
	static void __fin(void *allocated)          \
	{                                           \
		__pool.Free(allocated);                 \
	}                                           \
public:                                         \
    void* operator new(size_t count)            \
    {                                           \
        assert(sizeof(cls) == count);           \
		void *ptr = __pool.Alloc();             \
		*(unsigned int*) ptr = 0x80000000;      \
        return (unsigned int*) ptr + 1;         \
    }                                           \
    void operator delete(void *p)               \
    {                                           \
		unsigned int&cnt(*((unsigned int*)p-1));\
		cnt &= 0x7fffffff;                      \
		if( !cnt )                              \
			__pool.Free((unsigned int*) p - 1); \
		else                                    \
			*(ObjFinalizerProc*) p = __fin;     \
    }                                           \
	virtual int GetID() const                 \
    {                                           \
		return __pool.GetAllocID((unsigned int *) this - 1); \
    }

#define IMPLEMENT_POOLED_ALLOCATION(cls)        \
    MemoryPool<cls, sizeof(int)> cls::__pool;



/////////////////////////////////////////
// rtti and serialization

#define DECLARE_SELF_REGISTRATION(cls)          \
    DECLARE_POOLED_ALLOCATION(cls)              \
    private:                                    \
        typedef cls __ThisClass;                \
        static ObjectType _sType;               \
        static bool __registered;               \
        static bool __SelfRegister();           \
    public:                                     \
        static ObjectType GetTypeStatic()       \
        {                                       \
            return _sType;                      \
        }                                       \
        virtual ObjectType GetType()            \
        {                                       \
            return _sType;                      \
        }                                       \
    private:


#define IMPLEMENT_SELF_REGISTRATION(cls)                           \
    IMPLEMENT_POOLED_ALLOCATION(cls)                               \
	ObjectType cls::_sType = RTTypes::Inst().RegType<cls>(#cls);   \
    bool cls::__registered = cls::__SelfRegister();                \
    bool cls::__SelfRegister()


// for template classes (experimental)
#define IMPLEMENT_SELF_REGISTRATION_T(cls)                 \
    template<class T>                                      \
    ObjectType cls<T>::_sType = (cls<T>::__registered,     \
        RTTypes::Inst().__RegisterType<cls<T> >(typeid(cls<T>).name()));   \
    template<class T>                                      \
    bool cls<T>::__registered = cls<T>::__SelfRegister();  \
    template<class T>                                      \
    bool cls<T>::__SelfRegister()


///////////////////////////////////////////////////////////


class PropertySet : public RefCounted
{
	GC_Object       *_object;
	ObjectProperty   _propName;

protected:
	virtual void MyExchange(World &world, bool applyToObject);

public:
	PropertySet(GC_Object *object);

	GC_Object* GetObject() const;
	void LoadFromConfig();
	void SaveToConfig();
	void Exchange(World &world, bool applyToObject);

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
};

////////////////////////////////////////////////////////////
// object flags

// general
#define GC_FLAG_OBJECT_NAMED                  0x00000001

// engine events
#define GC_FLAG_OBJECT_EVENTS_TS_FIXED        0x00000002

#define GC_FLAG_OBJECT_                       0x00000004


typedef void (GC_Object::*NOTIFYPROC) (World &world, GC_Object *sender, void *param);

///////////////////////////////////////////////////////////////////////////////
class GC_Object
{
	GC_Object(const GC_Object&) = delete;
	GC_Object& operator = (const GC_Object&) = delete;

private:
	MemberOfGlobalList<LIST_objects> _memberOf;

	struct Notify
	{
//		DECLARE_POOLED_ALLOCATION(Notify);

		Notify *next;
		ObjPtr<GC_Object>   subscriber;
		NOTIFYPROC           handler;
		NotifyType           type;
		bool IsRemoved() const
		{
			return !subscriber;
		}
		void Serialize(World &world, SaveFile &f);
		explicit Notify(Notify *nxt) : next(nxt) {}
	};


	//
	// attributes
	//

private:
	unsigned int           _flags;             // define various object properties

	ObjectList::iterator _itPosFixed;      // position in the World::ts_fixed

	Notify *_firstNotify;
	int  _notifyProtectCount;

public:
	void SetFlags(unsigned int flags, bool value)
	{
		_flags = value ? (_flags|flags) : (_flags & ~flags);
	}
	unsigned int GetFlags() const
	{
		return _flags;
	}
	// return true if one of the flags is set
	bool CheckFlags(unsigned int flags) const
	{
		return 0 != (_flags & flags);
	}


	//
	// access functions
	//

public:


	//
	// construction/destruction
	//

	GC_Object();
	virtual ~GC_Object();


	//
	// operations
	//

protected:
	void PulseNotify(World &world, NotifyType type, void *param = NULL);

public:
	void SetEvents(World &world, unsigned int events);

	const char* GetName(World &world) const;
	void SetName(World &world, const char *name);

	void Subscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler);
	void Unsubscribe(NotifyType type, GC_Object *subscriber, NOTIFYPROC handler);


	//
	// serialization
	//

public:
	virtual void Serialize(World &world, SaveFile &f);

protected:
	GC_Object(FromFile);

public:
	virtual ObjectType GetType() = 0;


	//
	// properties
	//
protected:
	typedef PropertySet MyPropertySet;
	virtual PropertySet* NewPropertySet();

public:
	SafePtr<PropertySet> GetProperties(World &world);

	//
	// overrides
	//

public:
	virtual void Kill(World &world);

	virtual void TimeStepFixed(World &world, float dt);
	virtual void TimeStepFloat(World &world, float dt);
	virtual void EditorAction(World &world);

	virtual void MapExchange(World &world, MapFile &f);


	//
	// debug helpers
	//

#ifdef NETWORK_DEBUG
public:
	virtual uint32_t checksum(void) const
	{
		return 0;
	}
#endif
};

///////////////////////////////////////////////////////////////////////////////
// end of file

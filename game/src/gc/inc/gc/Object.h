#pragma once

#include "ObjectProperty.h"
#include "Serialization.h"
#include "detail/GlobalListHelper.h"
#include "detail/MemoryManager.h"

#include <memory>

class MapFile;
class SaveFile;

class GC_Object;
class World;

typedef unsigned int ObjectType;
#define INVALID_OBJECT_TYPE ((unsigned int) -1)


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
        virtual ObjectType GetType() const      \
        {                                       \
            return _sType;                      \
        }                                       \
    private:


class PropertySet
{
	GC_Object       &_object;
	ObjectProperty   _propName;

protected:
	virtual void MyExchange(World &world, bool applyToObject);

public:
	PropertySet(GC_Object *object);

	GC_Object* GetObject() const { return &_object; }
	void Exchange(World &world, bool applyToObject);

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);
};

////////////////////////////////////////////////////////////

#define GC_FLAG_OBJECT_NAMED                  0x00000001
#define GC_FLAG_OBJECT_                       0x00000002

typedef PtrList<GC_Object> ObjectList;
typedef void (GC_Object::*NOTIFYPROC) (World &world, GC_Object *sender, void *param);

class GC_Object
{
	GC_Object(const GC_Object&) = delete;
	GC_Object& operator = (const GC_Object&) = delete;
	DECLARE_LIST_MEMBER();

public:
	GC_Object();
	virtual ~GC_Object();
    
    ObjectList::id_type GetId() const { return _posLIST_objects; }

	const char* GetName(World &world) const;
	void SetName(World &world, const char *name);

	std::shared_ptr<PropertySet> GetProperties(World &world);

	virtual void Kill(World &world);
	virtual void MapExchange(MapFile &f);
	virtual void Init(World &world);
	virtual void Resume(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void TimeStep(World &world, float dt);
	virtual ObjectType GetType() const = 0;
#ifdef NETWORK_DEBUG
	virtual uint32_t checksum() const { return 0; }
#endif

protected:
	void SetFlags(unsigned int flags, bool value) { _flags = value ? (_flags|flags) : (_flags & ~flags); }
	unsigned int GetFlags() const { return _flags; }
	bool CheckFlags(unsigned int flags) const { return 0 != (_flags & flags); }

	typedef PropertySet MyPropertySet;
	virtual PropertySet* NewPropertySet();

private:
	unsigned int _flags;
	ObjectList::id_type _posLIST_objects;
};

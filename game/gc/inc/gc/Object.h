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
        ObjectType GetType() const override     \
        {                                       \
            return _sType;                      \
        }                                       \
    private:


class PropertySet
{
public:
	PropertySet(GC_Object *object);
	virtual ~PropertySet() = default;

	GC_Object* GetObject() const { return &_object; }
	void Exchange(World &world, bool applyToObject);

	virtual int GetCount() const;
	virtual ObjectProperty* GetProperty(int index);

protected:
	virtual void MyExchange(World &world, bool applyToObject);

private:
	GC_Object & _object;
	ObjectProperty   _propName;
};

////////////////////////////////////////////////////////////

#define GC_FLAG_OBJECT_NAMED                  0x00000001u
#define GC_FLAG_OBJECT_                       0x00000002u

typedef PtrList<GC_Object> ObjectList;

class GC_Object
{
	GC_Object(const GC_Object&) = delete;
	GC_Object& operator = (const GC_Object&) = delete;
	DECLARE_LIST_MEMBER(;);

public:
	GC_Object() = default;
	virtual ~GC_Object() = 0;

	ObjectList::id_type GetId() const { return _posLIST_objects; }

	std::string_view GetName(World &world) const;
	void SetName(World &world, std::string name);

	std::shared_ptr<PropertySet> GetProperties(World &world);

	virtual void Kill(World &world);
	virtual void MapExchange(MapFile &f);
	virtual void Serialize(World &world, SaveFile &f);
	virtual ObjectType GetType() const = 0;
#ifdef NETWORK_DEBUG
	virtual uint32_t checksum() const { return 0; }
#endif

private: // overrides don't have to call base class
	virtual void Init(World &world) {}
	virtual void Resume(World &world) {}
	virtual void TimeStep(World &world, float dt) {}

protected:
	void SetFlags(unsigned int flags, bool value) { _flags = value ? (_flags|flags) : (_flags & ~flags); }
	unsigned int GetFlags() const { return _flags; }
	bool CheckFlags(unsigned int flags) const { return 0 != (_flags & flags); }

	typedef PropertySet MyPropertySet;
	virtual PropertySet* NewPropertySet();

private:
	unsigned int _flags = 0;
	ObjectList::id_type _posLIST_objects;
};

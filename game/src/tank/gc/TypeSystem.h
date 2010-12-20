// TypeSystem.h

#pragma once

class GC_Object;
struct FromFile {};

class RTTypes
{
	struct EdItem
	{
		GC_Object* (*Create) (float, float);
		int           layer;
		float         align;
		float         offset;
		vec2d         size;
		bool          service;
		const char*   name;
		const char*   desc;
	};

	typedef std::map<ObjectType, EdItem> type2item;
	typedef std::map<string_t, ObjectType> name2type;
	typedef std::vector<ObjectType> index2type;
	typedef std::map<ObjectType, GC_Object* (*) ()> FromFileMap;

	template<class T> static GC_Object* ActorCtor(float x, float y)   { return new T(x, y); }
	template<class T> static GC_Object* ServiceCtor(float x, float y) { return new T(); }
	template<class T> static GC_Object* FromFileCtor() { return new T(::FromFile()); }

public:
	// access to singleton instance
	static RTTypes& Inst()
	{
		if( !_theInstance )
			_theInstance = new RTTypes();
		return *_theInstance;
	}


	//
	// type registration
	//

	template<class T>
	void RegisterActor( const char *name, const char *desc, int layer, float width,
	                    float height, float align, float offset )
	{
		assert( !IsRegistered(T::GetTypeStatic()) );
		assert( 0 == _n2t.count(name) );
		EdItem ei;
		ei.desc    = desc;  // index in localization table
		ei.name    = name;
		ei.layer   = layer;
		ei.size    = vec2d(width, height);
		ei.align   = align;
		ei.offset  = offset;
		ei.service = false;
		ei.Create  = ActorCtor<T>;
		_t2i[T::GetTypeStatic()] = ei;
		_n2t[name] = T::GetTypeStatic();
		_i2t.push_back(T::GetTypeStatic());
	}
	template<class T>
	void RegisterService( const char *name, const char *desc )
	{
		assert( !IsRegistered(T::GetTypeStatic()) );
		assert( 0 == _n2t.count(name) );
		EdItem ei = {0};
		ei.desc    = desc;
		ei.name    = name;
		ei.service = true;
		ei.Create  = ServiceCtor<T>;
		_t2i[T::GetTypeStatic()] = ei;
		_n2t[name] = T::GetTypeStatic();
		_i2t.push_back(T::GetTypeStatic());
	}

	template<class T>
	ObjectType RegType(const char *name)
	{
		// common
		assert(!_types.count(name));
		ObjectType type = (ObjectType) _types.size();
		_types.insert(name);
		// for serialization
		assert(!_ffm.count(type));
		_ffm[type] = FromFileCtor<T>;
		return type;
	}


	//
	// access type info
	//

	int GetTypeCount()
	{
		return _i2t.size();
	}
	const EdItem& GetTypeInfoByIndex(int typeIndex)
	{
		return _t2i[_i2t[typeIndex]];
	}
	const EdItem& GetTypeInfo(ObjectType type)
	{
		return _t2i[type];
	}
	ObjectType GetTypeByIndex(int typeIndex)
	{
		return _i2t[typeIndex];
	}
	ObjectType GetTypeByName(const string_t &name)
	{
		name2type::const_iterator it = _n2t.find(name);
		return _n2t.end() != it ? it->second : INVALID_OBJECT_TYPE;
	}
	const char* GetTypeName(ObjectType type)
	{
		assert(IsRegistered(type));
		return _t2i[type].name;
	}
	bool IsRegistered(ObjectType type)
	{
		return _t2i.find(type) != _t2i.end();
	}


	//
	// object creation
	//

	GC_Object* CreateFromFile(ObjectType type); // for serialization
	GC_Object* CreateObject(ObjectType type, float x, float y); // for editor


private:
	// for editor
	type2item _t2i;
	name2type _n2t;
	index2type _i2t; // sort by desc
	// for serialization
	FromFileMap _ffm;
	// common
	std::set<string_t> _types;
	// use as singleton only
	RTTypes() {};
	static RTTypes *_theInstance;
};

///////////////////////////////////////////////////////////////////////////////

#define ED_SERVICE(name, desc) RTTypes::Inst().RegisterService<__ThisClass>((name), (desc))

#define ED_ACTOR(name, desc, layer, width, height, align, offset)   \
	RTTypes::Inst().RegisterActor<__ThisClass>(                     \
	(name), (desc), (layer), (width), (height), (align), (offset) )

#define ED_ITEM(name, desc, layer) ED_ACTOR(name, desc, layer, 2, 2, CELL_SIZE/2, 0)
#define ED_LAND(name, desc, layer) ED_ACTOR(name, desc, layer, CELL_SIZE, CELL_SIZE, CELL_SIZE, CELL_SIZE/2)
#define ED_TURRET(name, desc) ED_ACTOR(name, desc, 0, CELL_SIZE*2, CELL_SIZE*2, CELL_SIZE, CELL_SIZE)



// end of file

// Variant.h

#pragma once

#define VARIANT_DEBUG

class DataStream
{
public:
	DataStream(bool serialize);
	~DataStream();

	bool Direction() const; // true - serialize, false - restore
	void Serialize(void *data, int bytes);
	void EntityBegin();
	void EntityEnd();
	bool EntityProbe() const;
	bool IsEmpty() const;

	int Send(SOCKET s);
	int Recv(SOCKET s);

private:
	typedef unsigned short EntitySizeType;
	std::vector<char> _buffer;
	int _entityLevel;
	size_t _entitySizeOffset;
	z_stream _z;
	bool _serialization;
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct VariantTypeIdHelper
{
};

template <typename T>
inline int VariantTypeId(T * = 0) 
{
	return VariantTypeIdHelper<T>::GetTypeId();
}

template <typename T>
void VariantMetaDesctructor(T *obj) 
{
	delete obj;
}

template <typename T>
void *VariantMetaConstructor(const T *copy)
{
	return copy ? new T(*copy) : new T;
}

template <typename T>
void VariantSerializeOp(DataStream &s, T *obj) 
{
	s & *obj;
}


template <typename T>
int VariantRegisterMetaType(T * = 0)
{
	return Variant::RegisterType(
		reinterpret_cast<Variant::Constructor>(VariantMetaConstructor<T>),
		reinterpret_cast<Variant::Destructor>(VariantMetaDesctructor<T>),
		reinterpret_cast<Variant::Serialize>(VariantSerializeOp<T>));
}


#define VARIANT_DECLARE_TYPE(type)                                      \
	template <>                                                         \
	struct VariantTypeIdHelper< type >                                  \
	{                                                                   \
		static bool registered;                                         \
		static Variant::TypeId GetTypeId()                              \
		{                                                               \
			static Variant::TypeId typeId = -1;                         \
			if( -1 == typeId )                                          \
			{                                                           \
				typeId = VariantRegisterMetaType< type >();             \
			}                                                           \
			return typeId;                                              \
		}                                                               \
	};                                                                  \
	DataStream& operator & (DataStream &, type &);


#define VARIANT_IMPLEMENT_TYPE(type)                                    \
	bool VariantTypeIdHelper< type >::registered =                      \
	    Variant::DeclareType(VariantTypeIdHelper< type >::GetTypeId);   \
	DataStream& operator & (DataStream &s, type &value)

#define RAW { s.Serialize(&value, sizeof(value)); return s; }

#define STD_VECTOR                                                      \
{                                                                       \
	assert(value.size() < 0xffff);                                     \
	unsigned short count = value.size();                                \
	s & count;                                                          \
	value.resize(count);                                                \
	for( size_t i = 0; i < value.size(); ++i )                          \
	{                                                                   \
		s & value[i];                                                   \
	}                                                                   \
	return s;                                                           \
}


class Variant
{
public:
	typedef int TypeId;
	typedef void  (*Serialize)(DataStream &s, void *);
	typedef void  (*Destructor)(void *);
	typedef void* (*Constructor)(const void *);

	Variant();
	template<class T>
	explicit Variant(const T &val)
	  : _type(VariantTypeId<T>())
	  , _data(_types[VariantTypeId<T>()].ctor(&val))
	{
	}

	Variant(const Variant &src);
	Variant& operator = (const Variant &src);


	~Variant();

	void ChangeType(TypeId type);

	template<class T> T& Value()
	{
		assert(_type == VariantTypeId<T>());
		return *reinterpret_cast<T *>(_data);
	}

	template<class T> const T& Value() const
	{
		assert(_type == VariantTypeId<T>());
		return *reinterpret_cast<const T *>(_data);
	}

	friend DataStream& operator & (DataStream &s, Variant &value)
	{
		s.EntityBegin();
#ifdef VARIANT_DEBUG
		TypeId tmp = value._type;
		s.Serialize(&tmp, sizeof(tmp));
		assert(tmp == value._type);
#endif
		value._types[value._type].ser(s, value._data);
		s.EntityEnd();
		return s;
	}


	static void Init();
	static TypeId RegisterType(Constructor ctor, Destructor dtor, Serialize ser);
	static bool DeclareType(TypeId (*declarator)());


private:
	Variant(TypeId type, const void *copyFrom);
	void Clear();

	struct UserType
	{
		Constructor ctor;
		Destructor dtor;
		Serialize ser;
	};

	static std::vector<UserType> _types;

	typedef std::vector<TypeId (*)()> Declarators;
	static Declarators& GetDecl();


	void *_data;
	TypeId _type;
#ifdef VARIANT_DEBUG
	static bool _reg;
	static bool _init;
#endif
};

///////////////////////////////////////////////////////////////////////////////
// basic types

VARIANT_DECLARE_TYPE(bool);
VARIANT_DECLARE_TYPE(char);
VARIANT_DECLARE_TYPE(short);
VARIANT_DECLARE_TYPE(int);
VARIANT_DECLARE_TYPE(long);
VARIANT_DECLARE_TYPE(long long);
VARIANT_DECLARE_TYPE(unsigned char);
VARIANT_DECLARE_TYPE(unsigned short);
VARIANT_DECLARE_TYPE(unsigned int);
VARIANT_DECLARE_TYPE(unsigned long);
VARIANT_DECLARE_TYPE(unsigned long long);
VARIANT_DECLARE_TYPE(float);
VARIANT_DECLARE_TYPE(double);

VARIANT_DECLARE_TYPE(std::string);

// end of file

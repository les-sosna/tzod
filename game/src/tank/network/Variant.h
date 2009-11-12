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

	int Send(SOCKET s, size_t *outSent = NULL);
	int Recv(SOCKET s);

	size_t GetTraffic() const;
	size_t GetPending() const { return _buffer.size(); }

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
	return VariantTypeIdHelper<T>::t.GetTypeId();
}

#define VARIANT_DECLARE_TYPE(T)                                         \
    DataStream& operator & (DataStream &, T &);                         \
    template <>                                                         \
    struct VariantTypeIdHelper< T >                                     \
    {                                                                   \
        class TypeRegHelper                                             \
        {                                                               \
            Variant::TypeId _typeId;                                    \
            TypeRegHelper(const TypeRegHelper &);                       \
            TypeRegHelper& operator = (const TypeRegHelper &);          \
            static void* Ctor(const void *other)                        \
            {                                                           \
                return other ?                                          \
                    new T(*reinterpret_cast<const T*>(other)) : new T;  \
            }                                                           \
            static void Dtor(void *obj)                                 \
            {                                                           \
                delete reinterpret_cast<T*>(obj);                       \
            }                                                           \
            static void Serialize(DataStream &s, void *obj)             \
            {                                                           \
                s & *reinterpret_cast<T*>(obj);                         \
            }                                                           \
        public:                                                         \
            TypeRegHelper() : _typeId(-1)                               \
            {                                                           \
                Variant::ScheduleTypeRegistration(                      \
                    Ctor, Dtor, Serialize, &_typeId);                   \
            }                                                           \
            inline Variant::TypeId GetTypeId() const                    \
            {                                                           \
                assert(-1 != _typeId);                                  \
                return _typeId;                                         \
            }                                                           \
        };                                                              \
        static TypeRegHelper t;                                         \
    };


#define VARIANT_IMPLEMENT_TYPE(type)                                            \
    VariantTypeIdHelper< type >::TypeRegHelper VariantTypeIdHelper< type >::t;  \
    DataStream& operator & (DataStream &s, type &value)

#define RAW { s.Serialize(&value, sizeof(value)); return s; }

#define STD_VECTOR                                                      \
{                                                                       \
	assert(value.size() < 0xffff);                                      \
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
	typedef void  (*DtorType)(void *);
	typedef void* (*CtorType)(const void *);

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
		value._types[value._type].serialize(s, value._data);
		s.EntityEnd();
		return s;
	}


	static void Init();
	static void ScheduleTypeRegistration(CtorType ctor, DtorType dtor, Serialize ser, TypeId *result);


private:
	Variant(TypeId type, const void *copyFrom);
	void Clear();

	struct UserType
	{
		CtorType ctor;
		DtorType dtor;
		Serialize serialize;
	};

	static std::vector<UserType> _types;

	typedef std::pair<TypeId*, UserType> RegData;
	typedef std::list<RegData> PendingRegList;
	static PendingRegList& GetPendingRegs();


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

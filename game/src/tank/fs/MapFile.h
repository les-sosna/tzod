// MapFile.h

#pragma once

///////////////////////////////////////////////////////////////////////////////

#define MAP_EXCHANGE_FLOAT(name, value, def_val)            \
	if( f.loading() )                                       \
	{                                                       \
	    if( !f.getObjectAttribute(#name, (float&) value) )  \
	        value = def_val;                                \
	}                                                       \
	else                                                    \
	{                                                       \
	    f.setObjectAttribute(#name, (float) value);         \
	}

#define MAP_EXCHANGE_INT(name, value, def_val)              \
	if( f.loading() )                                       \
	{                                                       \
	    if( !f.getObjectAttribute(#name, (int&) value) )    \
	        value = def_val;                                \
	}                                                       \
	else                                                    \
	{                                                       \
	    f.setObjectAttribute(#name, (int) value);           \
	}

////////////////////////////////////////////////////////////


#define SIGNATURE(s) ( (unsigned long) (                \
	((((unsigned long) (s)) & 0x000000ff) << 24)|       \
	((((unsigned long) (s)) & 0x0000ff00) <<  8)|       \
	((((unsigned long) (s)) & 0xff000000) >> 24)|       \
	((((unsigned long) (s)) & 0x00ff0000) >>  8)  ) )


class MapFile
{
	enum enumChunkTypes
	{
		CHUNK_HEADER_OPEN  = SIGNATURE('hdr{'),
		CHUNK_ATTRIB       = SIGNATURE('attr'),
		CHUNK_HEADER_CLOSE = SIGNATURE('}hdr'),
		CHUNK_OBJDEF       = SIGNATURE('dfn:'),
		CHUNK_OBJECT       = SIGNATURE('obj:'),
		CHUNK_TEXTURE      = SIGNATURE('tex:'),
		//------------------
		CHUNK_FORCE32BIT   = 0x7fffffff
	};

	enum enumDataTypes
	{
		DATATYPE_INT    = 1,
		DATATYPE_FLOAT  = 2,
		DATATYPE_STRING = 3,  // max length is 0xffff
		DATATYPE_RAW    = 4,  // max length is 0xffffffff
		//------------------
		DATATYPE_FORCE32BIT = 0x7fffffff
	};


	typedef struct ChunkHeader{
        enumChunkTypes chunkType;
		size_t         chunkSize;
	} ChunkHeader;

	typedef struct AttributeSet
	{
		std::map<string_t, int>         attrs_int;
		std::map<string_t, float>       attrs_float;
		std::map<string_t, string_t> attrs_str;

		void clear()
		{
			attrs_int.clear();
			attrs_float.clear();
			attrs_str.clear();
		}
	} AttributeSet;

	class ObjectDefinition
	{
	public:
		class Property
		{
		public:
			enumDataTypes type;
			string_t   name;
			Property() {}
			Property(const Property &x)
			{
				name = x.name;
				type = x.type;
			}
			size_t CalcSize() const
			{
				return sizeof(enumDataTypes) + sizeof(unsigned short) + name.size();
			}
		};

		string_t           _className;
		std::vector<Property> _propertyset;

		ObjectDefinition() {}
		ObjectDefinition(const ObjectDefinition &x)
		{
			_className   = x._className;
			_propertyset = x._propertyset;
		}

		size_t CalcSize() const
		{
			size_t size = sizeof(unsigned short) + _className.size();
            for( size_t i = 0; i < _propertyset.size(); i++ )
				size += _propertyset[i].CalcSize();
			return size;
		}
	};

private:
	std::ostringstream _buffer;

	FILE  *_file;
	bool   _modeWrite;
	bool   _isNewClass;

	AttributeSet _mapAttrs;

	std::vector<ObjectDefinition> _managed_classes;
	std::map<string_t, size_t> _name_to_index; // map classname to index in _managed_classes

	AttributeSet _obj_attrs;
	size_t  _obj_type; // index in _managed_classes

	std::map<string_t, AttributeSet> _defaults;


	bool _read_chunk_header(ChunkHeader &chdr);
	bool _write_chunk_header(const ChunkHeader &chdr);
	bool _skip_block(size_t size);


	bool WriteInt(int value);
	bool WriteFloat(float value);
	bool WriteString(const string_t &value);

	bool ReadInt(int &value);
	bool ReadFloat(float &value);
	bool ReadString(string_t &value);

public:
	MapFile(void);
	~MapFile(void);

	bool Open(const char *filename, bool write);
	bool Close();
	bool is_open() const;

	bool loading() const;


	bool NextObject();
	const char* getCurrentClassName() const;


	void BeginObject(const char *classname);
	bool WriteCurrentObject();


	bool getMapAttribute(const char *name, int &value) const;
	bool getMapAttribute(const char *name, float &value) const;
	bool getMapAttribute(const char *name, string_t &value) const;

	void setMapAttribute(const char *name, int value);
	void setMapAttribute(const char *name, float value);
	void setMapAttribute(const char *name, const string_t &value);


	bool getObjectAttribute(const char *name, int &value) const;
	bool getObjectAttribute(const char *name, float &value) const;
	bool getObjectAttribute(const char *name, string_t &value) const;

	void setObjectAttribute(const char *name, int value);
	void setObjectAttribute(const char *name, float value);
	void setObjectAttribute(const char *name, const string_t &value);

	void setObjectDefault(const char *cls, const char *attr, int value);
	void setObjectDefault(const char *cls, const char *attr, float value);
	void setObjectDefault(const char *cls, const char *attr, const string_t &value);

	template<class T>
	void Exchange(const char *name, T *value, T defaultValue)
	{
		_ASSERT(value)
		if( loading() )
		{
			if( !getObjectAttribute(name, *value) )
				value = defaultValue;
		}
		else
		{
			setObjectAttribute(name, *value);
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// end of file

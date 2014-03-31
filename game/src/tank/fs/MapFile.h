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

#define MAP_EXCHANGE_STRING(name, value, def_val)           \
	if( f.loading() )                                       \
	{                                                       \
	    if( !f.getObjectAttribute(#name, value) )           \
	        value = def_val;                                \
	}                                                       \
	else                                                    \
	{                                                       \
	    f.setObjectAttribute(#name, value);                 \
	}


////////////////////////////////////////////////////////////


#define SIGNATURE(s) ( (unsigned long) (                \
	((((unsigned long) (s)) & 0x000000ff) << 24)|       \
	((((unsigned long) (s)) & 0x0000ff00) <<  8)|       \
	((((unsigned long) (s)) & 0xff000000) >> 24)|       \
	((((unsigned long) (s)) & 0x00ff0000) >>  8)  ) )

namespace FS
{
	class Stream;
}


class MapFile
{
#pragma warning(push)
#ifdef __INTEL_COMPILER
# pragma warning(disable: 1899)
#endif
	enum enumChunkTypes : uint32_t
	{
		CHUNK_HEADER_OPEN  = SIGNATURE('hdr{'),
		CHUNK_ATTRIB       = SIGNATURE('attr'),
		CHUNK_HEADER_CLOSE = SIGNATURE('}hdr'),
		CHUNK_OBJDEF       = SIGNATURE('dfn:'),
		CHUNK_OBJECT       = SIGNATURE('obj:'),
		CHUNK_TEXTURE      = SIGNATURE('tex:'),
	};
#pragma warning(pop)

	enum enumDataTypes : uint32_t
	{
		DATATYPE_INT    = 1,
		DATATYPE_FLOAT  = 2,
		DATATYPE_STRING = 3,  // max length is 0xffff
		DATATYPE_RAW    = 4,  // max length is 0xffffffff
	};


	struct ChunkHeader
	{
		enumChunkTypes chunkType;
		uint32_t       chunkSize;
	};

	struct AttributeSet
	{
		std::map<std::string, int>      attrs_int;
		std::map<std::string, float>    attrs_float;
		std::map<std::string, std::string> attrs_str;

		void clear()
		{
			attrs_int.clear();
			attrs_float.clear();
			attrs_str.clear();
		}
	};

	class ObjectDefinition
	{
	public:
		class Property
		{
		public:
			enumDataTypes type;
			std::string   name;
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

		std::string           _className;
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

	SafePtr<FS::Stream> _file;
	bool   _modeWrite;
	bool   _headerWritten;
	bool   _isNewClass;

	AttributeSet _mapAttrs;

	std::vector<ObjectDefinition> _managed_classes;
	std::map<std::string, size_t> _name_to_index; // map classname to index in _managed_classes

	AttributeSet _obj_attrs;
	int  _obj_type; // index in _managed_classes

	std::map<std::string, AttributeSet> _defaults;


	bool _read_chunk_header(ChunkHeader &chdr);
	void _skip_block(size_t size);

	void WriteHeader();

	void WriteInt(int value);
	void WriteFloat(float value);
	void WriteString(const std::string &value);

	void ReadInt(int &value);
	void ReadFloat(float &value);
	void ReadString(std::string &value);

public:
	MapFile(const SafePtr<FS::Stream> &file, bool write);
	~MapFile();

	bool loading() const;


	bool NextObject();
	const std::string& GetCurrentClassName() const;


	void BeginObject(const char *classname);
	void WriteCurrentObject();


	bool getMapAttribute(const std::string &name, int &value) const;
	bool getMapAttribute(const std::string &name, float &value) const;
	bool getMapAttribute(const std::string &name, std::string &value) const;

	void setMapAttribute(const std::string &name, int value);
	void setMapAttribute(const std::string &name, float value);
	void setMapAttribute(const std::string &name, const std::string &value);


	bool getObjectAttribute(const std::string &name, int &value) const;
	bool getObjectAttribute(const std::string &name, float &value) const;
	bool getObjectAttribute(const std::string &name, std::string &value) const;

	void setObjectAttribute(const std::string &name, int value);
	void setObjectAttribute(const std::string &name, float value);
	void setObjectAttribute(const std::string &name, const std::string &value);

	void setObjectDefault(const char *cls, const char *attr, int value);
	void setObjectDefault(const char *cls, const char *attr, float value);
	void setObjectDefault(const char *cls, const char *attr, const std::string &value);

	template<class T>
	void Exchange(const char *name, T *value, T defaultValue)
	{
		assert(value);
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

private:
	MapFile(const MapFile&);
	MapFile& operator = (const MapFile&);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

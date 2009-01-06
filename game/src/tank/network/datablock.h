// datablock.h
////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////////

enum DbType
{
	DBTYPE_TEXTMESSAGE,
	DBTYPE_ERRORMSG,
	DBTYPE_GAMEINFO,
	DBTYPE_YOURID,
	DBTYPE_NEWPLAYER,
	DBTYPE_NEWBOT,
	DBTYPE_PLAYERINFO,
	DBTYPE_PLAYERQUIT,
//	DBTYPE_KICK,
	DBTYPE_SERVERQUIT,
	DBTYPE_PLAYERREADY,
	DBTYPE_STARTGAME,
	DBTYPE_CONTROLPACKET,
	DBTYPE_PING,
	DBTYPE_CHECKSUM,
	//----------------------
	DBTYPE_UNKNOWN = 0xffff
};

struct dbPlayerReady
{
	DWORD player_id;
	BOOL  ready;
};


////////////////////////////////////////////////////


class DataBlock
{
public:
	typedef unsigned short size_type;
	typedef unsigned short type_type;

	DataBlock();
	DataBlock(size_t dataSize);
	DataBlock(const DataBlock &src);
	~DataBlock();

	DataBlock& operator= (const DataBlock& src);
	size_t Parse(void *data, size_t size);


	size_type  DataSize()  const { return h().rawSize - sizeof(Header); }
	type_type& type() { return h().type; }
	const type_type& type() const { return h().type; }
	void*      Data()  const { return (char*) _data + sizeof(Header); }

	const char*  RawData()  const { return (const char *) _data; }
	size_t RawSize()  const { return h().rawSize; }

	template<class T> T& cast() const
	{
		_ASSERT(sizeof(T) == DataSize());
		return *(T*) Data();
	}

	template<class T> T& cast(size_t index) const
	{
		_ASSERT(0 == DataSize() % sizeof(T));
		_ASSERT(sizeof(T) * index <= DataSize());
		return ((T*) Data())[index];
	}

private:
	void *_data;

	struct Header
	{
		size_type rawSize; // size including header
		type_type type;
	};

	Header& h() { return *((Header*) _data); }
	const Header& h() const { return *((const Header*) _data); }
};

////////////////////////////////////////////////////

template<class T>
DataBlock DataWrap(T &data, DataBlock::type_type type)
{
	DataBlock db(sizeof(T));
	db.type() = type;
	memcpy(db.Data(), &data, sizeof(T));
	return db;
}

template<>
DataBlock DataWrap<const std::string>(const std::string &data, DataBlock::type_type type);

template<>
DataBlock DataWrap<std::string>(std::string &data, DataBlock::type_type type);

///////////////////////////////////////////////////////////////////////////////
// end of file

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

private:
	size_t _raw_size;
	void *_data;

	struct _header
	{
		size_type size;
		type_type type;
	};

	_header& h() { return *((_header*) _data); }


public:
	DataBlock();
	DataBlock(size_type size);
	DataBlock(const DataBlock &src);
	~DataBlock();

	DataBlock& operator= (const DataBlock& src);
	bool from_stream(void **pointer, size_t *stream_len);

public:
	size_type  size()  const { return ((_header*) _data)->size; }
	type_type& type()  const { return ((_header*) _data)->type; }
	void*      data()  const { return (char*) _data + sizeof(_header); }

	void*  raw_data()  const { return _data; }
	size_t raw_size()  const { return _raw_size; }

	template<class T> T& cast() const { return *((T*) data()); }
	template<class T> T& cast(size_t index) const { return ((T*) data())[index]; }
};

////////////////////////////////////////////////////

template<class T>
DataBlock DataWrap(T& data, DataBlock::type_type type)
{
	DataBlock db(sizeof(T));
	db.type() = type;
    memcpy(db.data(), &data, sizeof(T));
	return db;
}


///////////////////////////////////////////////////////////////////////////////
// end of file

// datablock.h
////////////////////////////////////////////////////

#pragma once


/////////////////////////////////////////////////////////

#define DBTYPE_UNKNOWN	-1


#define DBTYPE_TEXTMESSAGE	1
#define DBTYPE_ERRORMSG		2
#define DBTYPE_GAMEINFO		3
#define DBTYPE_YOURID		4
#define DBTYPE_NEWPLAYER	5
#define DBTYPE_PLAYERQUIT	6
//#define DBTYPE_KICK		7
#define DBTYPE_SERVERQUIT	8

#define DBTYPE_PLAYERREADY	9
struct dbPlayerReady
{
	DWORD player_id;
	BOOL  ready;
};

#define DBTYPE_STARTGAME	10
#define DBTYPE_CONTROLPACKET	11

#define DBTYPE_PING			12

#define DBTYPE_CHECKSUM			13

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

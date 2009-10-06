// KeyMapper.cpp

#include "stdafx.h"

#include "KeyMapper.h"

class KeyMapper
{
	std::map<string_t, int> _name2code;
	std::map<int, string_t> _code2name;

	void Pair(const char *name, int code);

public:
	KeyMapper();

	inline string_t GetName(int code) const;
	inline int GetCode(const string_t &name) const;
};

static KeyMapper s_keyMapper;

string_t GetKeyName(int code)
{
	return s_keyMapper.GetName(code);
}

int GetKeyCode(const string_t &name)
{
	return s_keyMapper.GetCode(name);
}


///////////////////////////////////////////////////////////////////////////////

KeyMapper::KeyMapper()
{
	Pair( "Escape",           1 );
	Pair( "1",                2 );
	Pair( "2",                3 );
	Pair( "3",                4 );
	Pair( "4",                5 );
	Pair( "5",                6 );
	Pair( "6",                7 );
	Pair( "7",                8 );
	Pair( "8",                9 );
	Pair( "9",               10 );
	Pair( "0",               11 );
	Pair( "-",               12 );
	Pair( "=",               13 );
	Pair( "Backspace",       14 );
	Pair( "Tab",             15 );
	Pair( "Q",               16 );
	Pair( "W",               17 );
	Pair( "E",               18 );
	Pair( "R",               19 );
	Pair( "T",               20 );
	Pair( "Y",               21 );
	Pair( "U",               22 );
	Pair( "I",               23 );
	Pair( "O",               24 );
	Pair( "P",               25 );
	Pair( "[",               26 );
	Pair( "]",               27 );
	Pair( "Enter",           28 );
	Pair( "Left Ctrl",       29 );
	Pair( "A",               30 );
	Pair( "S",               31 );
	Pair( "D",               32 );
	Pair( "F",               33 );
	Pair( "G",               34 );
	Pair( "H",               35 );
	Pair( "J",               36 );
	Pair( "K",               37 );
	Pair( "L",               38 );
	Pair( ";",               39 );
	Pair( "'",               40 );
	Pair( "~",               41 );
	Pair( "Left Shift",      42 );
	Pair( "\\",              43 );
	Pair( "Z",               44 );
	Pair( "X",               45 );
	Pair( "C",               46 );
	Pair( "V",               47 );
	Pair( "B",               48 );
	Pair( "N",               49 );
	Pair( "M",               50 );
	Pair( ",",               51 );
	Pair( ".",               52 );
	Pair( "/",               53 );
	Pair( "Right Shift",     54 );
	Pair( "Numpad *",        55 );
	Pair( "Left Alt",        56 );
	Pair( "Space",           57 );
	Pair( "Caps Lock",       58 );
	Pair( "F1",              59 );
	Pair( "F2",              60 );
	Pair( "F3",              61 );
	Pair( "F4",              62 );
	Pair( "F5",              63 );
	Pair( "F6",              64 );
	Pair( "F7",              65 );
	Pair( "F8",              66 );
	Pair( "F9",              67 );
	Pair( "F10",             68 );
	Pair( "Num Lock",        69 );
	Pair( "Scroll Lock",     70 );
	Pair( "Numpad 7",        71 );
	Pair( "Numpad 8",        72 );
	Pair( "Numpad 9",        73 );
	Pair( "Numpad -",        74 );
	Pair( "Numpad 4",        75 );
	Pair( "Numpad 5",        76 );
	Pair( "Numpad 6",        77 );
	Pair( "Numpad +",        78 );
	Pair( "Numpad 1",        79 );
	Pair( "Numpad 2",        80 );
	Pair( "Numpad 3",        81 );
	Pair( "Numpad 0",        82 );
	Pair( "Numpad .",        83 );
	Pair( "F11",             87 );
	Pair( "F12",             88 );
	Pair( "Numpad Enter",   156 );
	Pair( "Right Ctrl",     157 );
	Pair( "Numpad /",       181 );
	Pair( "Print Screen",   183 );
	Pair( "Right Alt",      184 );
	Pair( "Pause",          197 );
	Pair( "Home",           199 );
	Pair( "Up Arrow",       200 );
	Pair( "Page Up",        201 );
	Pair( "Left Arrow",     203 );
	Pair( "Right Arrow",    205 );
	Pair( "End",            207 );
	Pair( "Down Arrow",     208 );
	Pair( "Page Down",      209 );
	Pair( "Insert",         210 );
	Pair( "Delete",         211 );
	Pair( "Left Win",       219 );
	Pair( "Right Win",      220 );
	Pair( "App Menu",       221 );
	Pair( "System Power",   222 );
	Pair( "System Sleep",   223 );
	Pair( "System Wake",    227 );
	Pair( "Web Search",     229 );
	Pair( "Web Favorites",  230 );
	Pair( "Web Refresh",    231 );
	Pair( "Web Stop",       232 );
	Pair( "Web Forward",    233 );
	Pair( "Web Back",       234 );
	Pair( "My Computer",    235 );
	Pair( "Mail",           236 );
	Pair( "Media Select",   237 );

	Pair( "Mouse 1",        256 );
	Pair( "Mouse 2",        257 );
	Pair( "Mouse 3",        258 );
	Pair( "Mouse 4",        259 );
	Pair( "Mouse 5",        260 );
	Pair( "Mouse 6",        261 );
	Pair( "Mouse 7",        262 );
	Pair( "Mouse Wheel Up",   270 );
	Pair( "Mouse Wheel Down", 271 );
}

void KeyMapper::Pair(const char *name, int code)
{
	_code2name[code] = name;
	_name2code[name] = code;
}

string_t KeyMapper::GetName(int code) const
{
	std::map<int, string_t>::const_iterator it = _code2name.find(code);
	if( _code2name.end() == it )
	{
		char buf[8];
		wsprintf(buf, "#%03d", code);
		return buf;
	}

	return it->second;
}

int KeyMapper::GetCode(const string_t &name) const
{
	if( '#' == name[0] )
	{
		return atoi(name.c_str() + 1);
	}

	std::map<string_t, int>::const_iterator it = _name2code.find(name);
	if( _name2code.end() != it )
	{
		return it->second;
	}

	return -1;
}

// end of file

// KeyMapper.cpp

#include "KeyMapper.h"
#include <GLFW/glfw3.h>
#include <map>
#include <sstream>
#include <iomanip>

class KeyMapper
{
	std::map<std::string, int> _name2code;
	std::map<int, std::string> _code2name;

	void Pair(const char *name, int code);

public:
	KeyMapper();

	inline std::string GetName(int code) const;
	inline int GetCode(const std::string &name) const;
};

static KeyMapper s_keyMapper;

std::string GetKeyName(int code)
{
	return s_keyMapper.GetName(code);
}

int GetKeyCode(const std::string &name)
{
	return s_keyMapper.GetCode(name);
}


///////////////////////////////////////////////////////////////////////////////

KeyMapper::KeyMapper()
{
	Pair( "Escape",          GLFW_KEY_ESCAPE );
	Pair( "1",               GLFW_KEY_1 );
	Pair( "2",               GLFW_KEY_2 );
	Pair( "3",               GLFW_KEY_3 );
	Pair( "4",               GLFW_KEY_4 );
	Pair( "5",               GLFW_KEY_5 );
	Pair( "6",               GLFW_KEY_6 );
	Pair( "7",               GLFW_KEY_7 );
	Pair( "8",               GLFW_KEY_8 );
	Pair( "9",               GLFW_KEY_9 );
	Pair( "0",               GLFW_KEY_0 );
	Pair( "-",               GLFW_KEY_MINUS );
	Pair( "=",               GLFW_KEY_EQUAL );
	Pair( "Backspace",       GLFW_KEY_BACKSPACE );
	Pair( "Tab",             GLFW_KEY_TAB );
	Pair( "Q",               GLFW_KEY_Q );
	Pair( "W",               GLFW_KEY_W );
	Pair( "E",               GLFW_KEY_E );
	Pair( "R",               GLFW_KEY_R );
	Pair( "T",               GLFW_KEY_T );
	Pair( "Y",               GLFW_KEY_Y );
	Pair( "U",               GLFW_KEY_U );
	Pair( "I",               GLFW_KEY_I );
	Pair( "O",               GLFW_KEY_O );
	Pair( "P",               GLFW_KEY_P );
	Pair( "[",               GLFW_KEY_LEFT_BRACKET );
	Pair( "]",               GLFW_KEY_RIGHT_BRACKET );
	Pair( "Enter",           GLFW_KEY_ENTER );
	Pair( "Left Ctrl",       GLFW_KEY_LEFT_CONTROL );
	Pair( "A",               GLFW_KEY_A );
	Pair( "S",               GLFW_KEY_S );
	Pair( "D",               GLFW_KEY_D );
	Pair( "F",               GLFW_KEY_F );
	Pair( "G",               GLFW_KEY_G );
	Pair( "H",               GLFW_KEY_H );
	Pair( "J",               GLFW_KEY_J );
	Pair( "K",               GLFW_KEY_K );
	Pair( "L",               GLFW_KEY_L );
	Pair( ";",               GLFW_KEY_SEMICOLON );
	Pair( "'",               GLFW_KEY_APOSTROPHE );
	Pair( "~",               GLFW_KEY_GRAVE_ACCENT );
	Pair( "Left Shift",      GLFW_KEY_LEFT_SHIFT );
	Pair( "\\",              GLFW_KEY_BACKSLASH );
	Pair( "Z",               GLFW_KEY_Z );
	Pair( "X",               GLFW_KEY_X );
	Pair( "C",               GLFW_KEY_C );
	Pair( "V",               GLFW_KEY_V );
	Pair( "B",               GLFW_KEY_B );
	Pair( "N",               GLFW_KEY_N );
	Pair( "M",               GLFW_KEY_M );
	Pair( ",",               GLFW_KEY_COMMA );
	Pair( ".",               GLFW_KEY_PERIOD );
	Pair( "/",               GLFW_KEY_SLASH );
	Pair( "Right Shift",     GLFW_KEY_RIGHT_SHIFT );
	Pair( "Numpad *",        GLFW_KEY_KP_MULTIPLY );
	Pair( "Left Alt",        GLFW_KEY_LEFT_ALT );
	Pair( "Space",           GLFW_KEY_SPACE );
	Pair( "Caps Lock",       GLFW_KEY_CAPS_LOCK );
	Pair( "F1",              GLFW_KEY_F1 );
	Pair( "F2",              GLFW_KEY_F2 );
	Pair( "F3",              GLFW_KEY_F3 );
	Pair( "F4",              GLFW_KEY_F4 );
	Pair( "F5",              GLFW_KEY_F5 );
	Pair( "F6",              GLFW_KEY_F6 );
	Pair( "F7",              GLFW_KEY_F7 );
	Pair( "F8",              GLFW_KEY_F8 );
	Pair( "F9",              GLFW_KEY_F9 );
	Pair( "F10",             GLFW_KEY_F10 );
	Pair( "Num Lock",        GLFW_KEY_NUM_LOCK );
	Pair( "Scroll Lock",     GLFW_KEY_SCROLL_LOCK );
	Pair( "Numpad 7",        GLFW_KEY_KP_7 );
	Pair( "Numpad 8",        GLFW_KEY_KP_8 );
	Pair( "Numpad 9",        GLFW_KEY_KP_9 );
	Pair( "Numpad -",        GLFW_KEY_KP_SUBTRACT );
	Pair( "Numpad 4",        GLFW_KEY_KP_4 );
	Pair( "Numpad 5",        GLFW_KEY_KP_5 );
	Pair( "Numpad 6",        GLFW_KEY_KP_6 );
	Pair( "Numpad +",        GLFW_KEY_KP_ADD );
	Pair( "Numpad 1",        GLFW_KEY_KP_1 );
	Pair( "Numpad 2",        GLFW_KEY_KP_2 );
	Pair( "Numpad 3",        GLFW_KEY_KP_3 );
	Pair( "Numpad 0",        GLFW_KEY_KP_0 );
	Pair( "Numpad .",        GLFW_KEY_KP_DECIMAL );
	Pair( "F11",             GLFW_KEY_F11 );
	Pair( "F12",             GLFW_KEY_F12 );
	Pair( "Numpad Enter",    GLFW_KEY_KP_ENTER );
	Pair( "Right Ctrl",     GLFW_KEY_RIGHT_CONTROL );
	Pair( "Numpad /",       GLFW_KEY_KP_DIVIDE );
	Pair( "Print Screen",   GLFW_KEY_PRINT_SCREEN );
	Pair( "Right Alt",      GLFW_KEY_RIGHT_ALT );
	Pair( "Pause",          GLFW_KEY_PAUSE );
	Pair( "Home",           GLFW_KEY_HOME );
	Pair( "Up",             GLFW_KEY_UP );
	Pair( "Page Up",        GLFW_KEY_PAGE_UP );
	Pair( "Left",           GLFW_KEY_LEFT );
	Pair( "Right",          GLFW_KEY_RIGHT );
	Pair( "End",            GLFW_KEY_END );
	Pair( "Down",           GLFW_KEY_DOWN );
	Pair( "Page Down",      GLFW_KEY_PAGE_DOWN );
	Pair( "Insert",         GLFW_KEY_INSERT );
	Pair( "Delete",         GLFW_KEY_DELETE );
//	Pair( "Left Win",       GLFW_KEY_ );
//	Pair( "Right Win",      GLFW_KEY_ );
	Pair( "Menu",           GLFW_KEY_MENU );
//	Pair( "System Power",   GLFW_KEY_ );
//	Pair( "System Sleep",   GLFW_KEY_ );
//	Pair( "System Wake",    GLFW_KEY_ );
//	Pair( "Web Search",     GLFW_KEY_ );
//	Pair( "Web Favorites",  GLFW_KEY_ );
//	Pair( "Web Refresh",    GLFW_KEY_ );
//	Pair( "Web Stop",       GLFW_KEY_ );
//	Pair( "Web Forward",    GLFW_KEY_ );
//	Pair( "Web Back",       GLFW_KEY_ );
//	Pair( "My Computer",    GLFW_KEY_ );
//	Pair( "Mail",           GLFW_KEY_ );
//	Pair( "Media Select",   GLFW_KEY_ );

//	Pair( "Mouse 1",        256 );
//	Pair( "Mouse 2",        257 );
//	Pair( "Mouse 3",        258 );
//	Pair( "Mouse 4",        259 );
//	Pair( "Mouse 5",        260 );
//	Pair( "Mouse 6",        261 );
//	Pair( "Mouse 7",        262 );
//	Pair( "Mouse Wheel Up",   270 );
//	Pair( "Mouse Wheel Down", 271 );
}

void KeyMapper::Pair(const char *name, int code)
{
	_code2name[code] = name;
	_name2code[name] = code;
}

std::string KeyMapper::GetName(int code) const
{
	std::map<int, std::string>::const_iterator it = _code2name.find(code);
	if( _code2name.end() == it )
	{
		std::ostringstream buf;
		buf << '#' << std::setfill('0') << std::setw(3) << code;
		return buf.str();
	}

	return it->second;
}

int KeyMapper::GetCode(const std::string &name) const
{
	if( '#' == name[0] )
	{
        return atoi(name.c_str() + 1);
	}

	std::map<std::string, int>::const_iterator it = _name2code.find(name);
	if( _name2code.end() != it )
	{
		return it->second;
	}

	return -1;
}

// end of file

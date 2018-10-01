#include "KeyMapper.h"
#include <plat/Keys.h>
#include <map>
#include <sstream>
#include <iomanip>

class KeyMapper final
{
public:
	KeyMapper();

	inline std::string_view GetName(Plat::Key code) const;
	inline Plat::Key GetCode(std::string_view name) const;

private:
	std::map<std::string, Plat::Key, std::less<>> _name2code;
	std::map<Plat::Key, std::string> _code2name;

	void Pair(const char *name, Plat::Key code);
};

static KeyMapper s_keyMapper;

std::string_view GetKeyName(Plat::Key code)
{
	return s_keyMapper.GetName(code);
}

Plat::Key GetKeyCode(std::string_view name)
{
	return s_keyMapper.GetCode(name);
}


///////////////////////////////////////////////////////////////////////////////

KeyMapper::KeyMapper()
{
	Pair( "Escape",          Plat::Key::Escape );
	Pair( "1",               Plat::Key::_1 );
	Pair( "2",               Plat::Key::_2 );
	Pair( "3",               Plat::Key::_3 );
	Pair( "4",               Plat::Key::_4 );
	Pair( "5",               Plat::Key::_5 );
	Pair( "6",               Plat::Key::_6 );
	Pair( "7",               Plat::Key::_7 );
	Pair( "8",               Plat::Key::_8 );
	Pair( "9",               Plat::Key::_9 );
	Pair( "0",               Plat::Key::_0 );
	Pair( "-",               Plat::Key::Minus );
	Pair( "=",               Plat::Key::Equal );
	Pair( "Backspace",       Plat::Key::Backspace );
	Pair( "Tab",             Plat::Key::Tab );
	Pair( "Q",               Plat::Key::Q );
	Pair( "W",               Plat::Key::W );
	Pair( "E",               Plat::Key::E );
	Pair( "R",               Plat::Key::R );
	Pair( "T",               Plat::Key::T );
	Pair( "Y",               Plat::Key::Y );
	Pair( "U",               Plat::Key::U );
	Pair( "I",               Plat::Key::I );
	Pair( "O",               Plat::Key::O );
	Pair( "P",               Plat::Key::P );
	Pair( "[",               Plat::Key::LeftBracket );
	Pair( "]",               Plat::Key::RightBracket );
	Pair( "Enter",           Plat::Key::Enter );
	Pair( "Left Ctrl",       Plat::Key::LeftCtrl );
	Pair( "A",               Plat::Key::A );
	Pair( "S",               Plat::Key::S );
	Pair( "D",               Plat::Key::D );
	Pair( "F",               Plat::Key::F );
	Pair( "G",               Plat::Key::G );
	Pair( "H",               Plat::Key::H );
	Pair( "J",               Plat::Key::J );
	Pair( "K",               Plat::Key::K );
	Pair( "L",               Plat::Key::L );
	Pair( ";",               Plat::Key::Semicolon );
	Pair( "'",               Plat::Key::Apostrophe );
	Pair( "~",               Plat::Key::GraveAccent );
	Pair( "Left Shift",      Plat::Key::LeftShift );
	Pair( "\\",              Plat::Key::Backslash );
	Pair( "Z",               Plat::Key::Z );
	Pair( "X",               Plat::Key::X );
	Pair( "C",               Plat::Key::C );
	Pair( "V",               Plat::Key::V );
	Pair( "B",               Plat::Key::B );
	Pair( "N",               Plat::Key::N );
	Pair( "M",               Plat::Key::M );
	Pair( ",",               Plat::Key::Comma );
	Pair( ".",               Plat::Key::Period );
	Pair( "/",               Plat::Key::Slash );
	Pair( "Right Shift",     Plat::Key::RightShift );
	Pair( "Numpad *",        Plat::Key::NumStar );
	Pair( "Left Alt",        Plat::Key::LeftAlt );
	Pair( "Space",           Plat::Key::Space );
	Pair( "Caps Lock",       Plat::Key::CapsLock );
	Pair( "F1",              Plat::Key::F1 );
	Pair( "F2",              Plat::Key::F2 );
	Pair( "F3",              Plat::Key::F3 );
	Pair( "F4",              Plat::Key::F4 );
	Pair( "F5",              Plat::Key::F5 );
	Pair( "F6",              Plat::Key::F6 );
	Pair( "F7",              Plat::Key::F7 );
	Pair( "F8",              Plat::Key::F8 );
	Pair( "F9",              Plat::Key::F9 );
	Pair( "F10",             Plat::Key::F10 );
	Pair( "Num Lock",        Plat::Key::NumLock );
	Pair( "Scroll Lock",     Plat::Key::ScrollLock );
	Pair( "Numpad 7",        Plat::Key::Num7 );
	Pair( "Numpad 8",        Plat::Key::Num8 );
	Pair( "Numpad 9",        Plat::Key::Num9 );
	Pair( "Numpad -",        Plat::Key::NumMinus );
	Pair( "Numpad 4",        Plat::Key::Num4 );
	Pair( "Numpad 5",        Plat::Key::Num5 );
	Pair( "Numpad 6",        Plat::Key::Num6 );
	Pair( "Numpad +",        Plat::Key::NumPlus );
	Pair( "Numpad 1",        Plat::Key::Num1 );
	Pair( "Numpad 2",        Plat::Key::Num2 );
	Pair( "Numpad 3",        Plat::Key::Num3 );
	Pair( "Numpad 0",        Plat::Key::Num0 );
	Pair( "Numpad .",        Plat::Key::NumPeriod );
	Pair( "F11",             Plat::Key::F11 );
	Pair( "F12",             Plat::Key::F12 );
	Pair( "Numpad Enter",    Plat::Key::NumEnter );
	Pair( "Right Ctrl",      Plat::Key::RightCtrl );
	Pair( "Numpad /",        Plat::Key::NumSlash );
	Pair( "Print Screen",    Plat::Key::PrintScreen );
	Pair( "Right Alt",       Plat::Key::RightAlt );
	Pair( "Pause",           Plat::Key::Pause );
	Pair( "Home",            Plat::Key::Home );
	Pair( "Up",              Plat::Key::Up );
	Pair( "Page Up",         Plat::Key::PageUp );
	Pair( "Left",            Plat::Key::Left );
	Pair( "Right",           Plat::Key::Right );
	Pair( "End",             Plat::Key::End );
	Pair( "Down",            Plat::Key::Down );
	Pair( "Page Down",       Plat::Key::PageDown );
	Pair( "Insert",          Plat::Key::Insert );
	Pair( "Delete",          Plat::Key::Delete );
	Pair( "Menu",            Plat::Key::Menu );


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

void KeyMapper::Pair(const char *name, Plat::Key code)
{
	_code2name[code] = name;
	_name2code[name] = code;
}

std::string_view KeyMapper::GetName(Plat::Key code) const
{
	auto it = _code2name.find(code);
	if( _code2name.end() == it )
	{
		static std::string unknown("unknown");
		return unknown;
	}

	return it->second;
}

Plat::Key KeyMapper::GetCode(std::string_view name) const
{
	auto it = _name2code.find(name);
	return _name2code.end() != it ? it->second : Plat::Key::Unknown;
}

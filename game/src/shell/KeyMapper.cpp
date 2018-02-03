#include "KeyMapper.h"
#include <ui/Keys.h>
#include <map>
#include <sstream>
#include <iomanip>

class KeyMapper
{
public:
	KeyMapper();

	inline std::string_view GetName(UI::Key code) const;
	inline UI::Key GetCode(std::string_view name) const;

private:
	std::map<std::string, UI::Key, std::less<>> _name2code;
	std::map<UI::Key, std::string> _code2name;

	void Pair(const char *name, UI::Key code);
};

static KeyMapper s_keyMapper;

std::string_view GetKeyName(UI::Key code)
{
	return s_keyMapper.GetName(code);
}

UI::Key GetKeyCode(std::string_view name)
{
	return s_keyMapper.GetCode(name);
}


///////////////////////////////////////////////////////////////////////////////

KeyMapper::KeyMapper()
{
	Pair( "Escape",          UI::Key::Escape );
	Pair( "1",               UI::Key::_1 );
	Pair( "2",               UI::Key::_2 );
	Pair( "3",               UI::Key::_3 );
	Pair( "4",               UI::Key::_4 );
	Pair( "5",               UI::Key::_5 );
	Pair( "6",               UI::Key::_6 );
	Pair( "7",               UI::Key::_7 );
	Pair( "8",               UI::Key::_8 );
	Pair( "9",               UI::Key::_9 );
	Pair( "0",               UI::Key::_0 );
	Pair( "-",               UI::Key::Minus );
	Pair( "=",               UI::Key::Equal );
	Pair( "Backspace",       UI::Key::Backspace );
	Pair( "Tab",             UI::Key::Tab );
	Pair( "Q",               UI::Key::Q );
	Pair( "W",               UI::Key::W );
	Pair( "E",               UI::Key::E );
	Pair( "R",               UI::Key::R );
	Pair( "T",               UI::Key::T );
	Pair( "Y",               UI::Key::Y );
	Pair( "U",               UI::Key::U );
	Pair( "I",               UI::Key::I );
	Pair( "O",               UI::Key::O );
	Pair( "P",               UI::Key::P );
	Pair( "[",               UI::Key::LeftBracket );
	Pair( "]",               UI::Key::RightBracket );
	Pair( "Enter",           UI::Key::Enter );
	Pair( "Left Ctrl",       UI::Key::LeftCtrl );
	Pair( "A",               UI::Key::A );
	Pair( "S",               UI::Key::S );
	Pair( "D",               UI::Key::D );
	Pair( "F",               UI::Key::F );
	Pair( "G",               UI::Key::G );
	Pair( "H",               UI::Key::H );
	Pair( "J",               UI::Key::J );
	Pair( "K",               UI::Key::K );
	Pair( "L",               UI::Key::L );
	Pair( ";",               UI::Key::Semicolon );
	Pair( "'",               UI::Key::Apostrophe );
	Pair( "~",               UI::Key::GraveAccent );
	Pair( "Left Shift",      UI::Key::LeftShift );
	Pair( "\\",              UI::Key::Backslash );
	Pair( "Z",               UI::Key::Z );
	Pair( "X",               UI::Key::X );
	Pair( "C",               UI::Key::C );
	Pair( "V",               UI::Key::V );
	Pair( "B",               UI::Key::B );
	Pair( "N",               UI::Key::N );
	Pair( "M",               UI::Key::M );
	Pair( ",",               UI::Key::Comma );
	Pair( ".",               UI::Key::Period );
	Pair( "/",               UI::Key::Slash );
	Pair( "Right Shift",     UI::Key::RightShift );
	Pair( "Numpad *",        UI::Key::NumStar );
	Pair( "Left Alt",        UI::Key::LeftAlt );
	Pair( "Space",           UI::Key::Space );
	Pair( "Caps Lock",       UI::Key::CapsLock );
	Pair( "F1",              UI::Key::F1 );
	Pair( "F2",              UI::Key::F2 );
	Pair( "F3",              UI::Key::F3 );
	Pair( "F4",              UI::Key::F4 );
	Pair( "F5",              UI::Key::F5 );
	Pair( "F6",              UI::Key::F6 );
	Pair( "F7",              UI::Key::F7 );
	Pair( "F8",              UI::Key::F8 );
	Pair( "F9",              UI::Key::F9 );
	Pair( "F10",             UI::Key::F10 );
	Pair( "Num Lock",        UI::Key::NumLock );
	Pair( "Scroll Lock",     UI::Key::ScrollLock );
	Pair( "Numpad 7",        UI::Key::Num7 );
	Pair( "Numpad 8",        UI::Key::Num8 );
	Pair( "Numpad 9",        UI::Key::Num9 );
	Pair( "Numpad -",        UI::Key::NumMinus );
	Pair( "Numpad 4",        UI::Key::Num4 );
	Pair( "Numpad 5",        UI::Key::Num5 );
	Pair( "Numpad 6",        UI::Key::Num6 );
	Pair( "Numpad +",        UI::Key::NumPlus );
	Pair( "Numpad 1",        UI::Key::Num1 );
	Pair( "Numpad 2",        UI::Key::Num2 );
	Pair( "Numpad 3",        UI::Key::Num3 );
	Pair( "Numpad 0",        UI::Key::Num0 );
	Pair( "Numpad .",        UI::Key::NumPeriod );
	Pair( "F11",             UI::Key::F11 );
	Pair( "F12",             UI::Key::F12 );
	Pair( "Numpad Enter",    UI::Key::NumEnter );
	Pair( "Right Ctrl",      UI::Key::RightCtrl );
	Pair( "Numpad /",        UI::Key::NumSlash );
	Pair( "Print Screen",    UI::Key::PrintScreen );
	Pair( "Right Alt",       UI::Key::RightAlt );
	Pair( "Pause",           UI::Key::Pause );
	Pair( "Home",            UI::Key::Home );
	Pair( "Up",              UI::Key::Up );
	Pair( "Page Up",         UI::Key::PageUp );
	Pair( "Left",            UI::Key::Left );
	Pair( "Right",           UI::Key::Right );
	Pair( "End",             UI::Key::End );
	Pair( "Down",            UI::Key::Down );
	Pair( "Page Down",       UI::Key::PageDown );
	Pair( "Insert",          UI::Key::Insert );
	Pair( "Delete",          UI::Key::Delete );
	Pair( "Menu",            UI::Key::Menu );


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

void KeyMapper::Pair(const char *name, UI::Key code)
{
	_code2name[code] = name;
	_name2code[name] = code;
}

std::string_view KeyMapper::GetName(UI::Key code) const
{
	auto it = _code2name.find(code);
	if( _code2name.end() == it )
	{
		static std::string unknown("unknown");
		return unknown;
	}

	return it->second;
}

UI::Key KeyMapper::GetCode(std::string_view name) const
{
	auto it = _name2code.find(name);
	return _name2code.end() != it ? it->second : UI::Key::Unknown;
}

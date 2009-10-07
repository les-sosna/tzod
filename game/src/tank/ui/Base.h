// Base.h

#pragma once

#include <string>
#include <sstream>

namespace UI
{
	//
	// forward declarations of all GUI classes
	//

	class Window;
	class ButtonBase;
	class TextButton;
	class Button;
	class CheckBox;
	class ComboBox;
	class Dialog;
	class Edit;
	class ImageButton;
	class List;
	class MouseCursor;
	class ScrollBarBase;
	class ScrollBarVertical;
	class ScrollBarHorizontal;
	class Text;
	class Console;
	class ConsoleBuffer;
	class ConsoleHistoryDefault;
	class Oscilloscope;

#ifdef UNICODE
	typedef std::wstring string_t;
	typedef std::wostringstream ostrstream_t;
	typedef std::wistringstream istrstream_t;
#else
	typedef std::string string_t;
	typedef std::ostringstream ostrstream_t;
	typedef std::istringstream istrstream_t;
#endif

} // end of namespace UI

///////////////////////////////////////////////////////////////////////////////
// end of file

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

	struct ListDataSource;
	class ListDataSourceDefault;

	template <class DataSourceType, class ListType>
	class ListAdapter;

} // end of namespace UI

///////////////////////////////////////////////////////////////////////////////
// end of file

// Combo.h

#pragma once

#include "Window.h"


namespace UI
{
;

// forward declarations
class Text;
class List;
class ButtonBase;


///////////////////////////////////////////////////////////////////////////////
// ComboBox control

class ComboBox : public Window
{
	Text        *_text;
	List        *_list;
	ButtonBase  *_btn;

public:
	ComboBox(Window *parent, float x, float y, float width);

	List* GetList() const;
	void DropList();

	Delegate<void(void)> eventChangeCurSel;


protected:
	void OnSelectItem(int index);
};



///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file
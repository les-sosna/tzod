// List.h

#pragma once

#include "Base.h"
#include "Window.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// multi-column ListBox control

class List : public Window
{
public:
	struct Item
	{
		std::vector<string_t>  text;
		UINT_PTR               data;
	};

private:
	std::vector<Item>  _items;
	std::vector<float> _tabs;

	ScrollBar *_scrollBar;
	Text      *_blankText; // used for text drawing
	Window    *_selection;

	int        _curSel;

	void OnScroll(float pos);
	void UpdateSelection();

public:
	List(Window *parent, float x, float y, float width, float height);

	void DeleteAllItems();

	int      AddItem(const char *str, UINT_PTR data = 0);
	void     SetItemText(int index, int sub, const char *str);
	float    GetItemHeight() const;
	string_t GetItemText(int index, int sub) const;

	void SetTabPos(int index, float pos);

	void SetItemData(int index, ULONG_PTR data);
	ULONG_PTR GetItemData(int index);

	int  GetSize() const;
	int  GetCurSel() const;
	void SetCurSel(int sel, bool scroll = false);

	int  HitTest(float y); // returns index of item

	float GetNumLinesVisible() const;
	void ScrollTo(float pos);

	void AlignHeightToContent();

	Delegate<void(int)> eventChangeCurSel;
	Delegate<void(int)> eventClickItem;


protected:
	virtual void OnSize(float width, float height);
	virtual bool OnMouseDown(float x, float y, int button);
	virtual bool OnMouseWheel(float x, float y, float z);
	virtual void OnRawChar(int c);
	virtual bool OnFocus(bool focus);

	virtual void DrawChildren(float sx, float sy);
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

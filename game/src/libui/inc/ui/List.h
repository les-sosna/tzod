// List.h

#pragma once

#include "Window.h"
#include "ListBase.h"

#include <functional>

namespace UI
{

class ScrollBarVertical;


///////////////////////////////////////////////////////////////////////////////
// multi-column ListBox control

class List : public Window
{
public:
	static List* Create(Window *parent, ListDataSource* dataSource, float x, float y, float width, float height);

	ListDataSource* GetData() const;

	float GetScrollPos() const;
	void SetScrollPos(float pos);

	float GetItemHeight() const;
	float GetNumLinesVisible() const;
	void AlignHeightToContent(float maxHeight = 512);
	int HitTest(float y); // returns index of item

	int  GetCurSel() const;
	void SetCurSel(int sel, bool scroll = false);

	void SetTabPos(int index, float pos);

	// list events
	std::function<void(int)> eventChangeCurSel;
	std::function<void(int)> eventClickItem;

protected:
	List(Window *parent, ListDataSource* dataSource);
	virtual ~List();

	// callback interface
	class ListCallbackImpl : public ListDataSourceListener
	{
	public:
		ListCallbackImpl(List *list);
	private:
		virtual void OnDeleteAllItems();
		virtual void OnDeleteItem(int idx);
		virtual void OnAddItem();
		List *_list;
	};

	ListCallbackImpl _callbacks;

protected:
	void OnSize(float width, float height) override;
	bool OnPointerMove(float x, float y, PointerID pointerID) override;
	bool OnMouseLeave() override;
	bool OnPointerDown(float x, float y, int button, PointerID pointerID) override;
	bool OnPointerUp(float x, float y, int button, PointerID pointerID) override;
	bool OnMouseWheel(float x, float y, float z) override;
    bool OnTap(float x, float y) override;
	bool OnKeyPressed(Key key) override;
	bool OnFocus(bool focus) override;

	void DrawChildren(DrawingContext &dc, float sx, float sy) const override;

private:
	List(const List &); // no copy
	List& operator = (const List &);

	ListDataSource *_data;
	std::vector<float> _tabs;

	ScrollBarVertical *_scrollBar;

	int        _curSel;
	int        _hotItem;

	size_t     _font;
	size_t     _selection;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

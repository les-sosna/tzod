// List.h

#pragma once

#include "Base.h"
#include "Window.h"
#include "ListBase.h"

namespace UI
{

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
	Delegate<void(int)> eventChangeCurSel;
	Delegate<void(int)> eventClickItem;

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
	virtual void OnSize(float width, float height);
	virtual bool OnMouseMove(float x, float y);
	virtual bool OnMouseLeave();
	virtual bool OnMouseDown(float x, float y, int button);
	virtual bool OnMouseUp(float x, float y, int button);
	virtual bool OnMouseWheel(float x, float y, float z);
	virtual bool OnRawChar(int c);
	virtual bool OnFocus(bool focus);

	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

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

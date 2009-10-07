// List.h

#pragma once

#include "Base.h"
#include "Window.h"

namespace UI
{

///////////////////////////////////////////////////////////////////////////////
// multi-column ListBox control

interface ListDataSourceListener
{
	virtual void OnDeleteAllItems() = 0;
	virtual void OnDeleteItem(int idx) = 0;
	virtual void OnAddItem() = 0;
};

class ListDataSource : public RefCounted
{
public:
	virtual void SetListener(ListDataSourceListener *cb) = 0;

	virtual int GetItemCount() const = 0;
	virtual int GetSubItemCount(int index) const = 0;
	virtual ULONG_PTR GetItemData(int index) const = 0;
	virtual const string_t& GetItemText(int index, int sub) const = 0;
	virtual int FindItem(const string_t &text) const = 0;
};

class ListDataSourceDefault : public ListDataSource
{
public:
	// ListDataSource interface
	virtual void SetListener(ListDataSourceListener *cb);
	virtual int GetItemCount() const;
	virtual int GetSubItemCount(int index) const;
	virtual ULONG_PTR GetItemData(int index) const;
	virtual const string_t& GetItemText(int index, int sub) const;
	virtual int FindItem(const string_t &text) const;

	// extra
	int  AddItem(const string_t &str, UINT_PTR data = 0);
	void SetItemText(int index, int sub, const string_t &str);
	void SetItemData(int index, ULONG_PTR data);
	void DeleteItem(int index);
	void DeleteAllItems();
	void Sort();


private:
	struct Item
	{
		std::vector<string_t>  text;
		UINT_PTR               data;
	};
	std::vector<Item>  _items;
	ListDataSourceListener *_listener;
};


class List : public Window
{
public:
	static List* Create(Window *parent, float x, float y, float width, float height);
	virtual ~List();

	SafePtr<ListDataSourceDefault> GetDataDefault() const;
	SafePtr<ListDataSource> GetData() const;
	void SetData(const SafePtr<ListDataSource> &source);

	float GetScrollPos() const;
	void SetScrollPos(float pos);

	float GetItemHeight() const;
	float GetNumLinesVisible() const;
	void AlignHeightToContent(float maxHeight = 512);
	int HitTest(float y); // returns index of item

	int  GetCurSel() const;
	void SetCurSel(int sel, bool scroll = false);

	void SetTabPos(int index, float pos);

	// these functions are directed to ListDataSource interface
	int GetItemCount() const;
	int GetSubItemCount(int index) const;
	ULONG_PTR GetItemData(int index) const;
	const string_t& GetItemText(int index, int sub = 0) const;
	int FindItem(const string_t &text) const;

	// these functions are directed to ListDataSourceDefault interface (if available one)
	// to be removed!
	int  AddItem(const string_t &str, UINT_PTR data = 0);
	void SetItemText(int index, int sub, const string_t &str);
	void SetItemData(int index, ULONG_PTR data);
	void DeleteItem(int index);
	void DeleteAllItems();
	void Sort();

	// list events
	Delegate<void(int)> eventChangeCurSel;
	Delegate<void(int)> eventClickItem;

protected:
	List(Window *parent);

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
	virtual bool OnMouseWheel(float x, float y, float z);
	virtual bool OnRawChar(int c);
	virtual bool OnFocus(bool focus);

	virtual void DrawChildren(const DrawingContext *dc, float sx, float sy) const;

private:
	SafePtr<ListDataSource> _data;

	std::vector<float> _tabs;

	ScrollBarVertical *_scrollBar;
	size_t     _font;
	size_t     _selection;

	int        _curSel;
	int        _hotItem;
};


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

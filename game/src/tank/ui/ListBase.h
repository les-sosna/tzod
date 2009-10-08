// ListBase.h

#pragma once

namespace UI
{

struct ListDataSourceListener
{
	virtual void OnDeleteAllItems() = 0;
	virtual void OnDeleteItem(int idx) = 0;
	virtual void OnAddItem() = 0;
};

struct ListDataSource
{
	virtual void AddListener(ListDataSourceListener *cb) = 0;
	virtual void RemoveListener(ListDataSourceListener *cb) = 0;

	virtual int GetItemCount() const = 0;
	virtual int GetSubItemCount(int index) const = 0;
	virtual ULONG_PTR GetItemData(int index) const = 0;
	virtual const string_t& GetItemText(int index, int sub) const = 0;
	virtual int FindItem(const string_t &text) const = 0;
};

///////////////////////////////////////////////////////////////////////////////

class ListDataSourceDefault : public ListDataSource
{
public:
	// ListDataSource interface
	virtual void AddListener(ListDataSourceListener *cb);
	virtual void RemoveListener(ListDataSourceListener *cb);
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

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

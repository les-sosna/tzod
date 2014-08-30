// DataSourceAdapters.h

#pragma once

namespace UI
{

template <class DataSourceType, class ListType>
class ListAdapter
	: private DataSourceType
	, public ListType
{
public:
	template <class ...Args>
	static ListAdapter* Create(Window *parent, Args && ...args)
	{
		return new ListAdapter(parent, std::forward<Args>(args)...);
	}

	DataSourceType* GetData()
	{
		return this;
	}

protected:
	template <class ...Args>
	ListAdapter(Window *parent, Args && ...args)
		: DataSourceType(std::forward<Args>(args)...)
		, ListType(parent, this)
	{
	}
	virtual ~ListAdapter()
	{
	}
};


} // end of namespace UI
// end of file

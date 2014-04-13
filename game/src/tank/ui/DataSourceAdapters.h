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
	static ListAdapter* Create(Window *parent)
	{
		return new ListAdapter(parent);
	}

	DataSourceType* GetData()
	{
		return this;
	}

protected:
	ListAdapter(Window *parent)
		: DataSourceType()
		, ListType(parent, this)
	{
	}
	virtual ~ListAdapter()
	{
	}
};


} // end of namespace UI
// end of file

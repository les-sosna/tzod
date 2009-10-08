// DataSourceAdapters.h

#pragma once

#include "Base.h"

namespace UI
{

template <class DataSourceType, class ListType>
class ListAdapter : public ListType
{
public:
	static ListAdapter* Create(Window *parent)
	{
		return new ListAdapter(parent);
	}

	DataSourceType* GetData() const
	{
		return _data;
	}

protected:
	ListAdapter(Window *parent)
		: ListType(parent, new DataSourceType()) // FIXME: memory leak when ListType::ListType throws
		, _data(static_cast<DataSourceType*>(ListType::GetData()))
	{
	}
	virtual ~ListAdapter()
	{
		delete _data;
	}

private:
	DataSourceType *_data;
};


} // end of namespace UI
// end of file

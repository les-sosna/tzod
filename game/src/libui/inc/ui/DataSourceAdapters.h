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
	static std::shared_ptr<ListAdapter> Create(Window *parent, Args && ...args)
	{
		auto res = std::make_shared<ListAdapter>(parent->GetManager(), std::forward<Args>(args)...);
		parent->AddFront(res);
		return res;
	}

	DataSourceType* GetData()
	{
		return this;
	}

	template <class ...Args>
	explicit ListAdapter(LayoutManager &manager, Args && ...args)
		: DataSourceType(std::forward<Args>(args)...)
		, ListType(manager, this)
	{
	}
	virtual ~ListAdapter()
	{
	}
};


} // end of namespace UI
// end of file

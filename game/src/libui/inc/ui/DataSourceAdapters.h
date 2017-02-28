#pragma once

namespace UI
{
	template <class DataSourceType, class ListType>
	class ListAdapter
		: private DataSourceType
		, public ListType
	{
	public:
		DataSourceType* GetData()
		{
			return this;
		}

		template <class ...Args>
		ListAdapter(TextureManager &texman, Args && ...args)
			: DataSourceType(std::forward<Args>(args)...)
			, ListType(texman, this)
		{
		}
	};

	template <class DataSourceType, class ListType>
	class ListAdapterM
		: private DataSourceType
		, public ListType
	{
	public:
		DataSourceType* GetData()
		{
			return this;
		}

		template <class ...Args>
		ListAdapterM(LayoutManager &manager, TextureManager &texman, Args && ...args)
			: DataSourceType(std::forward<Args>(args)...)
			, ListType(manager, texman, this)
		{
		}
	};
}

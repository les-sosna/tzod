#pragma once

namespace UI
{
	class DataContext
	{
	public:
		const void* GetDataContext() const { return _dataContext; }
		void SetDataContext(const void *dataContext) { _dataContext = dataContext; }

		void SetItemIndex(unsigned int itemIndex) { _itemIndex = itemIndex; }
		unsigned int GetItemIndex() const { return _itemIndex; }

	private:
		const void *_dataContext = nullptr;
		unsigned int _itemIndex = 0;
	};
}

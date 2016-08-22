#pragma once
#include <string>

namespace UI
{
	class StateContext
	{
	public:
		void SetState(std::string state) { _state = state; }
		const std::string& GetState() const { return _state; }

		const void* GetDataContext() const { return _dataContext; }
		void SetDataContext(const void *dataContext) { _dataContext = dataContext; }

		void SetItemIndex(unsigned int itemIndex) { _itemIndex = itemIndex; }
		unsigned int GetItemIndex() const { return _itemIndex; }

	private:
		std::string _state;
		const void *_dataContext = nullptr;
		unsigned int _itemIndex = 0;
	};
}

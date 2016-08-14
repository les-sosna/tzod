#pragma once
#include <string>

namespace UI
{
	class StateContext
	{
	public:
		void SetState(std::string state) { _state = state; }
		const std::string& GetState() const { return _state; }

		void SetDataContext(const void *dataContext) { _dataContext = dataContext; }
		const void* GetDataContext() const { return _dataContext; }

	private:
		std::string _state;
		const void *_dataContext = nullptr;
	};
}

#pragma once
#include <string>

namespace UI
{
	class StateContext
	{
	public:
		void SetState(std::string state) { _state = state; }
		const std::string& GetState() const { return _state; }

	private:
		std::string _state;
	};
}

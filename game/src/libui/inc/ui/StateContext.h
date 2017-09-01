#pragma once
#include <string>
#include <string_view>
#include <utility>

namespace UI
{
	class StateContext
	{
	public:
		void SetState(std::string state) { _state = std::move(state); }
		std::string_view GetState() const { return _state; }

	private:
		std::string _state;
	};
}

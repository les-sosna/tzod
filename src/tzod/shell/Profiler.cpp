#include "inc/shell/Profiler.h"

std::vector<std::pair<CounterInfo, CounterBase*>>& CounterBase::GetRegisteredCountersStatic()
{
	static std::vector<std::pair<CounterInfo, CounterBase*>> v;
	return v;
}

size_t CounterBase::RegisterMarkerStatic(CounterInfo info, CounterBase *ptr)
{
	size_t result = GetRegisteredCountersStatic().size();
	GetRegisteredCountersStatic().emplace_back(std::move(info), ptr);
	return result;
}

size_t CounterBase::GetMarkerCountStatic()
{
	return GetRegisteredCountersStatic().size();
}

const CounterInfo& CounterBase::GetMarkerInfoStatic(size_t idx)
{
	return GetRegisteredCountersStatic()[idx].first;
}

void CounterBase::SetMarkerCallbackStatic(size_t idx, std::function<void(float)> cb)
{
	GetRegisteredCountersStatic()[idx].second->_callback = std::move(cb);
}

CounterBase::CounterBase(std::string id, std::string title)
{
	CounterInfo ci;
	ci.id = std::move(id);
	ci.title = std::move(title);
	RegisterMarkerStatic(std::move(ci), this);
}

void CounterBase::Push(float value)
{
	if( _callback )
		_callback(value);
}

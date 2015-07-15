#include "Profiler.h"

std::vector<CounterBase::CounterInfoEx>& CounterBase::GetRegisteredCountersStatic()
{
	static std::vector<CounterInfoEx> v;
	return v;
}

size_t CounterBase::RegisterMarkerStatic(const CounterInfo &info, CounterBase *ptr)
{
	size_t result = GetRegisteredCountersStatic().size();
	GetRegisteredCountersStatic().push_back(info);
	GetRegisteredCountersStatic().back().ptr = ptr;
	return result;
}

size_t CounterBase::GetMarkerCountStatic()
{
	return GetRegisteredCountersStatic().size();
}

const CounterInfo& CounterBase::GetMarkerInfoStatic(size_t idx)
{
	return GetRegisteredCountersStatic()[idx];
}

void CounterBase::SetMarkerCallbackStatic(size_t idx, std::function<void(float)> cb)
{
    GetRegisteredCountersStatic()[idx].ptr->_callback = std::move(cb);
}

CounterBase::CounterBase(const std::string &id, const std::string &title)
{
	CounterInfo ci;
	ci.id = id;
	ci.title = title;
	RegisterMarkerStatic(ci, this);
}

void CounterBase::Push(float value)
{
	if( _callback )
        _callback(value);
}

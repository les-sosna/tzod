#pragma once

#include <functional>
#include <string>
#include <vector>


struct CounterInfo
{
	std::string id;
	std::string title;
};

class CounterBase
{
public:
	CounterBase(std::string id, std::string title);
	void Push(float value);

	static size_t GetMarkerCountStatic();
	static const CounterInfo& GetMarkerInfoStatic(size_t idx);
	static void SetMarkerCallbackStatic(size_t idx, std::function<void(float)> cb);

private:
	std::function<void(float)> _callback;

	static size_t RegisterMarkerStatic(CounterInfo info, CounterBase *ptr);
	static std::vector<std::pair<CounterInfo, CounterBase*>>& GetRegisteredCountersStatic();
};

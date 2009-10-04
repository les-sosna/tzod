// Profiler.h 

#pragma once

struct CounterInfo
{
	std::string id;
	string_t title;
};

class CounterBase
{
public:
	CounterBase(const std::string &id, const string_t &title);
	void Push(float value);

	static size_t GetMarkerCountStatic();
	static const CounterInfo& GetMarkerInfoStatic(size_t idx);
	static void SetMarkerCallbackStatic(size_t idx, const Delegate<void(float)> &cb);

private:
	Delegate<void(float)> _callback;

	struct CounterInfoEx : public CounterInfo
	{
		CounterInfoEx(const CounterInfo &src): CounterInfo(src) {}
		CounterBase *ptr;
	};

	static size_t RegisterMarkerStatic(const CounterInfo &info, CounterBase *ptr);
	static std::vector<CounterInfoEx>& GetRegisteredCountersStatic();
};


#define DECLARE_PERFORMANCE_MARKER(id, title) CounterBase g_perf##id(#id, title)
#define INSERT_MARKER(id, value) g_perf##id.Push(value)


// end of file

#pragma once

#include <chrono>
#include <deque>

class Timer
{
    typedef std::chrono::high_resolution_clock clock;
    clock::time_point _time_pause;
    clock::time_point _time_last_dt;
    std::chrono::duration<float> _time_max_dt;
	std::deque<float> _movingAverageWindow;
	std::deque<float> _movingMedianWindow;

	int _stopCount;

public:
	Timer();  // initially stopped

public:
	float GetDt();
	void  SetMaxDt(float dt);

	void Start();
	void Stop();
	bool IsRuning() {return !_stopCount;}
};

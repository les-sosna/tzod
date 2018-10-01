#pragma once

#include <chrono>

class Timer final
{
public:
	Timer();  // initially stopped

	float GetDt();
	void  SetMaxDt(float dt);

	void Start();
	void Stop();
	bool IsRuning() {return !_stopCount;}

private:
	typedef std::chrono::high_resolution_clock clock;
	clock::time_point _time_pause;
	clock::time_point _time_last_dt;
	std::chrono::duration<float> _time_max_dt;

	int _stopCount;
};

#include "Timer.h"
#include <algorithm>
#include <cassert>
#include <numeric>

Timer::Timer()
	: _stopCount(1) // timer is stopped initially
{
	_time_max_dt = clock::duration::max();
	_time_last_dt = clock::now();
	_time_pause = _time_last_dt;
}

float Timer::GetDt()
{
	std::chrono::duration<float> fseconds;

	if( _stopCount )
	{
		fseconds = _time_pause - _time_last_dt;
		_time_last_dt = _time_pause;
	}
	else
	{
		auto current = clock::now();
		fseconds = current - _time_last_dt;
		_time_last_dt = current;
	}

	if( fseconds > _time_max_dt )
		fseconds = _time_max_dt;

	// moving average
	_movingAverageWindow.push_back(fseconds.count());
	if (_movingAverageWindow.size() > 8)
		_movingAverageWindow.pop_front();
	float mean = std::accumulate(_movingAverageWindow.begin(), _movingAverageWindow.end(), 0.0f) / (float)_movingAverageWindow.size();

	// moving median of moving average
	_movingMedianWindow.push_back(mean);
	if (_movingMedianWindow.size() > 100)
		_movingMedianWindow.pop_front();
	float buf[100];
	std::copy(_movingMedianWindow.begin(), _movingMedianWindow.end(), buf);
	std::nth_element(buf, buf + _movingMedianWindow.size() / 2, buf + _movingMedianWindow.size());

	// median
	return buf[_movingMedianWindow.size() / 2];
}


void Timer::SetMaxDt(float dt)
{
	_time_max_dt = std::chrono::duration<float>(dt);
}

void Timer::Stop()
{
	if( !_stopCount )
	{
		_time_pause = clock::now();
	}

	_stopCount++;
}

void Timer::Start()
{
	_stopCount--;
	assert(_stopCount >= 0);

	if( !_stopCount )
	{
		_time_last_dt += clock::now() - _time_pause;
	}
}

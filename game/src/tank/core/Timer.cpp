// Timer.cpp: implementation of the Timer class.

#include "Timer.h"
#include "Debug.h"

#include <cassert>


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

    return fseconds.count();
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
	TRACE("timer: stop;  now stop count is %d", _stopCount);
}

void Timer::Start()
{
	_stopCount--;
	TRACE("timer: start;  now stop count is %d", _stopCount);
	assert(_stopCount >= 0);

	if( !_stopCount )
	{
		_time_last_dt += clock::now() - _time_pause;
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

// Timer.cpp: implementation of the Timer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Timer.h"
#include "Debug.h"
#include "Application.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Timer::Timer()
{
	_qpf_time_max_dt.QuadPart = MAXLONGLONG;
	_stopCount = 1; // timer is stopped initially

	LARGE_INTEGER  f;
	QueryPerformanceFrequency(&f);
	_qpf_frequency = (double) f.QuadPart;

	QueryPerformanceCounter(&_qpf_time_last_dt);
	_qpf_time_pause = _qpf_time_last_dt;
}

float Timer::GetDt()
{
	LARGE_INTEGER  time;

	if( _stopCount )
	{
		time.QuadPart = _qpf_time_pause.QuadPart - _qpf_time_last_dt.QuadPart;
		_qpf_time_last_dt.QuadPart = _qpf_time_pause.QuadPart;
	}
	else
	{
		LARGE_INTEGER current;
		QueryPerformanceCounter(&current);

		time.QuadPart = current.QuadPart - _qpf_time_last_dt.QuadPart;
		_qpf_time_last_dt.QuadPart = current.QuadPart;
	}

	if( time.QuadPart > _qpf_time_max_dt.QuadPart )
		time = _qpf_time_max_dt;

	return (float)( (double) time.QuadPart / _qpf_frequency );
}


void Timer::SetMaxDt(float dt)
{
	_qpf_time_max_dt.QuadPart = (LONGLONG) ((double) dt * _qpf_frequency);
}

void Timer::Stop()
{
	if( !_stopCount )
	{
		QueryPerformanceCounter(&_qpf_time_pause);
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
		LARGE_INTEGER current;
		QueryPerformanceCounter(&current);
		_qpf_time_last_dt.QuadPart += current.QuadPart - _qpf_time_pause.QuadPart;
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

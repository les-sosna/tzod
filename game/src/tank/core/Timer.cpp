// Timer.cpp: implementation of the Timer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Timer.h"
#include "Debug.h"
#include "Console.h"
#include "Application.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Timer::Timer()
{
	_qpf_time_max_dt.QuadPart = MAXLONGLONG;
	_stopCount = 1; // изначально таймер остановлен

	LARGE_INTEGER  f;
	QueryPerformanceFrequency(&f);
	_qpf_frequency = (double) f.QuadPart;

	// по непонятным причинам в windows xp не работает
	// вызов QueryPerformanceCounter(&_qpf_time_last_dt)

	QueryPerformanceCounter(&f);
	_qpf_time_last_dt = f;
	_qpf_time_pause   = f;
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
		// по непонятным причинам в windows xp не работает прямой
		// вызов QueryPerformanceCounter(&_qpf_time_pause)

		LARGE_INTEGER tmp;
		QueryPerformanceCounter(&tmp);
		_qpf_time_pause = tmp;
	}

	_stopCount++;
	TRACE("timer: stop;  now stop count is %d\n", _stopCount);
}

void Timer::Start()
{
	_stopCount--;
	TRACE("timer: start;  now stop count is %d\n", _stopCount);
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

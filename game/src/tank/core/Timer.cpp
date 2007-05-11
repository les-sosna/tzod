// Timer.cpp: implementation of the Timer class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"

#include "Timer.h"
#include "Debug.h"
#include "Console.h"

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

Timer::Timer()
{
	TRACE("timer: init\n");

	_qpf_time_max_dt.QuadPart = MAXLONGLONG;
	_time_max_dt              = MAXLONG;

	_stopCount = 1; // изначально таймер остановлен

	// пытаемс€ использовать QPF
	LARGE_INTEGER  f;
	_useQPF = QueryPerformanceFrequency(&f);

	if( _useQPF )
	{
		TRACE("timer: using QPF\n");
		TRACE("timer: frequency is %I64u Hz\n", f.QuadPart);

		_qpf_frequency = (double) f.QuadPart;

		// по непон€тным причинам в windows xp не работает
		// вызов QueryPerformanceCounter(&_qpf_time_last_dt)

		QueryPerformanceCounter(&f);
		_qpf_time_last_dt = f;
		_qpf_time_pause   = f;
	}
	else
	{
		// QPF не поддерживаетс€. будем использовать timeGetTime
		timeBeginPeriod(1);
		TRACE("timer: using timeGetTime\n");
	}
}

Timer::~Timer()
{
	TRACE("timer: shotdown\n");
	if( !_useQPF )
	{
		timeEndPeriod(1);
	}
}


float Timer::GetDt()
{
	if( _useQPF )
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


	DWORD time;

	if( _stopCount )
	{
		time = _time_pause - _time_last_dt;
		_time_last_dt = _time_pause;
	}
	else
	{
		DWORD current = timeGetTime();

		time = current - _time_last_dt;
		_time_last_dt = current;
	}

	if( time > _time_max_dt )
		time = _time_max_dt;

	_ASSERT(time >= 0);
	return (0.001f * (float) time);
}


void Timer::SetMaxDt(float dt)
{
	_qpf_time_max_dt.QuadPart = (LONGLONG) ( (double) dt * _qpf_frequency );
	_time_max_dt              = int( dt * 1000.0f );
}

void Timer::Stop()
{
	if( !_stopCount )
	{
		if( _useQPF )
		{
			// по непон€тным причинам в windows xp не работает пр€мой
			// вызов QueryPerformanceCounter(&_qpf_time_pause)

			LARGE_INTEGER tmp;
			QueryPerformanceCounter(&tmp);
			_qpf_time_pause = tmp;
		}
		else
		{
			_time_pause = timeGetTime();
		}
	}

	_stopCount++;
	TRACE("timer: stop;  now stop count is %d\n", _stopCount);
}

void Timer::Start()
{
	_stopCount--;
	TRACE("timer: start;  now stop count is %d\n", _stopCount);
	_ASSERT(_stopCount >= 0);

	if( !_stopCount )
	{
		if( _useQPF )
		{
			LARGE_INTEGER current;
			QueryPerformanceCounter(&current);
			_qpf_time_last_dt.QuadPart += current.QuadPart - _qpf_time_pause.QuadPart;
		}
		else
		{
			DWORD current = timeGetTime();
			_time_last_dt += current - _time_pause;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

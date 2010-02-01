// Timer.h: interface for the Timer class.
//
//////////////////////////////////////////////////////////////////////

#pragma once

class Timer
{
protected:
	// qpf
	double        _qpf_frequency;
	LARGE_INTEGER _qpf_time_pause;
	LARGE_INTEGER _qpf_time_last_dt;
	LARGE_INTEGER _qpf_time_max_dt;

	LONG _stopCount;

public:
	Timer();  // initially stopped

public:
	float GetDt();
	void  SetMaxDt(float dt);

	void Start();
	void Stop();
	bool IsRuning() {return !_stopCount;}
};

///////////////////////////////////////////////////////////////////////////////
// end of file

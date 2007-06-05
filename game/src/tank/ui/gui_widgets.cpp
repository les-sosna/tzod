// gui_widgets.cpp

#include "stdafx.h"

#include "gui_widgets.h"

#include "ui/GuiManager.h"

#include "Level.h"

#include "network/TankClient.h"

namespace UI
{
///////////////////////////////////////////////////////////////////////////////
// FPS counter implementation

FpsCounter::FpsCounter(Window *parent, float x, float y, enumAlignText align)
  : Text(parent, x, y, "", align)
{
	_nSprites = 0;
	_nLights  = 0;
	_nBatches = 0;

	SetTimeStep(true);
}

void FpsCounter::OnShow(bool show)
{
	SetTimeStep(show);
}

//void FpsCounter::TimeStepFixed(float dt)
//{
//	_dts_net.push_back(_timer_net.GetDt());
//	if( _dts_net.size() > 100 ) _dts_net.pop_front();
//}

void FpsCounter::OnTimeStep(float dt)
{
	_dts.push_back(dt);
	if( _dts.size() > 200 ) _dts.pop_front();

	if( IsVisible() )
	{
		float avr = 0;
		float min = _dts.front();
		float max = _dts.front();

		for( std::list<float>::iterator it = _dts.begin(); it != _dts.end(); ++it )
		{
			avr += *it;
			if( *it > max ) max = *it;
			if( *it < min ) min = *it;
		}
		avr /= (float) _dts.size();

		char s[512];
		wsprintf(s, "%5dobj; %3dlight; %5dsprite; %2dbatch; %3d-%3d-%3dfps; %6dsprites/sec",
			-1/*g_level->objects.size()*/, _nLights,
			_nSprites, _nBatches,
			int(1.0f / max + 0.5f), int(1.0f / avr + 0.5f), int(1.0f / min + 0.5f),
			_nSprites * int(1.0f / avr + 0.5f));

		char s1[256];
		wsprintf(s1, "\nEvents: %4dfixed; %4dfloat; %4dendframe  Wnd: %3d total",
			-1/*g_level->ts_fixed.size()*/,
			-1/*g_level->ts_floating.size()*/,
			-1/*g_level->endframe.size()*/,
			g_gui->GetWndCount()
		);
		strcat(s, s1);

		// network statistics
		if( g_level->_client )
		{
			min = max = _dts_net.front();
			for( std::list<float>::iterator it = _dts_net.begin();
				 it != _dts_net.end(); ++it )
			{
				avr += *it;
				if( *it > max ) max = *it;
				if( *it < min ) min = *it;
			}
			avr /= (float) _dts_net.size();


			NETWORKSTATS ns;
			g_level->_client->GetStatistics(&ns);
			wsprintf(s1, "\nNetwork: %2dbuf; sent%3dk; recv%3dk; fps: %3dmin %3davr %3dmax;",
				ns.nFramesInBuffer, ns.dwBytesSent/1024, ns.dwBytesRecv/1024,
				int(1.0f / max + 0.5f), int(1.0f / avr + 0.5f), int(1.0f / min + 0.5f)
			);
			strcat(s, s1);
		}

		SetText(s);
	}

	_nSprites = 0;
	_nLights  = 0;
	_nBatches = 0;
}

///////////////////////////////////////////////////////////////////////////////

TimeElapsed::TimeElapsed(Window *parent, float x, float y, enumAlignText align)
  : Text(parent, x, y, "", align)
{
	SetTimeStep(true);
}

void TimeElapsed::OnShow(bool show)
{
	SetTimeStep(show);
}

void TimeElapsed::OnTimeStep(float dt)
{
	char text[16];
	int time = int(g_level->_time);

	if( time % 60 < 10 )
		wsprintf(text, "%d:0%d", time / 60, time % 60);
	else
		wsprintf(text, "%d:%d", time / 60, time % 60);

	SetText(text);
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

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

		char s [512];
		char s1[256];

		wsprintf(s, "fps:%04d-%04d-%04d; wnd:%03d",
			int(1.0f / max + 0.5f), 
			int(1.0f / avr + 0.5f), 
			int(1.0f / min + 0.5f),
			g_gui->GetWndCount()
		);

		wsprintf(s1, "; obj:%d\nevents: %4dfix %4dfloat %4def",
			g_level->GetList(LIST_objects).size(), 
			g_level->ts_fixed.size(),
			g_level->ts_floating.size(),
			g_level->endframe.size()
		);
		strcat(s, s1);

		// network statistics
		if( g_client )
		{
			if( _dts_net.empty() )
			{
				min = max = avr = 0;
			}
			else
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
			}

			NetworkStats ns;
			g_client->GetStatistics(&ns);
			wsprintf(s1, "\nnetwork: %2dbuf; sent%3dk; recv%3dk", // "; ms:%3d-%3d-%3d",
				/*ns.nFramesInBuffer*/-1, ns.bytesSent/1024, ns.bytesRecv/1024 //, min, avr, max
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
	if( !g_level->IsEmpty() )
	{
		char text[16];
		int time = int(g_level->_time);

		if( time % 60 < 10 )
			wsprintf(text, "%d:0%d", time / 60, time % 60);
		else
			wsprintf(text, "%d:%d", time / 60, time % 60);

		SetText(text);
	}
	else
	{
		SetText("--:--");
	}
}


///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

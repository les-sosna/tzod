// gui_widgets.cpp

#include "stdafx.h"

#include "gui_widgets.h"

#include "ui/GuiManager.h"
#include "video/TextureManager.h"

#include "Level.h"

#include "network/TankClient.h"
#include "network/TankServer.h"

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

void FpsCounter::OnVisibleChange(bool visible, bool inherited)
{
	SetTimeStep(visible);
}

void FpsCounter::OnTimeStep(float dt)
{
	_dts.push_back(dt);
	if( _dts.size() > 200 ) _dts.pop_front();

	if( GetVisible() )
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

		char s [1024];
		char s1[256];

		wsprintf(s, "fps:%04d-%04d-%04d; wnd:%03d",
			int(1.0f / max + 0.5f), 
			int(1.0f / avr + 0.5f), 
			int(1.0f / min + 0.5f),
			g_gui->GetWndCount()
		);

		wsprintf(s1, "; obj:%d\nevents: %4dfix %4dfloat",
			g_level->GetList(LIST_objects).size(), 
			g_level->ts_fixed.size(),
			g_level->ts_floating.size()
		);
		strcat(s, s1);


		if( g_server )
		{
			strcat(s, "\nsv - ");
			strcat(s, g_server->GetStats().c_str());
		}

		// network statistics
		if( g_client )
		{
			NetworkStats ns;
			g_client->GetStatistics(&ns);
			sprintf_s(s1, "\nsent:%uk; recv:%uk pending:%u timebuf:%f", 
				ns.bytesSent>>10, ns.bytesRecv>>10, ns.bytesPending, g_level->_timeBuffer);
			strcat(s, s1);


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

			sprintf_s(s1, "\nnet: %3.fdrop; lag %s", // "; ms:%3d-%3d-%3d",
				g_level->_dropedFrames, g_level->_lag.c_str() //, min, avr, max
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

void TimeElapsed::OnVisibleChange(bool visible, bool inherited)
{
	SetTimeStep(visible);
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

Oscilloscope::Oscilloscope(Window *parent, float x, float y)
  : Window(parent, x, y, "ui/list")
  , _barTexture(g_texman->FindSprite("ui/bar"))
  , _titleFont(g_texman->FindSprite("font_small"))
  , _rangeMin(-0.1f)
  , _rangeMax(0.1f)
  , _scale(3)
{
	SetBorder(true);
	SetClipChildren(true);
}

void Oscilloscope::Push(float value)
{
	_data.push_back(value);
	size_t size = (size_t) (GetWidth() / _scale);
	if( _data.size() > size )
	{
		_data.erase(_data.begin(), _data.begin() + (_data.size() - size));
	}
}

void Oscilloscope::SetRange(float rmin, float rmax)
{
	_rangeMin = rmin;
	_rangeMax = rmax;
}

void Oscilloscope::SetTitle(const string_t &title)
{
	_title = title;
}

void Oscilloscope::SetGrid(const float *data, size_t count)
{
	_vgrid.assign(data, data + count);
}

void Oscilloscope::DrawChildren(float sx, float sy) const
{
	float scale = GetHeight() / (_rangeMin - _rangeMax);
	float center = sy - _rangeMax * scale;
	float dx = sx + GetWidth() - (float) _data.size() * _scale;

	for( size_t i = 0; i < _data.size(); ++i )
	{
		g_texman->DrawSprite(_barTexture, 0, 0x44444444, (float) i * _scale + dx, center, 2, _data[i] * scale, 0);
	}

	for( std::vector<float>::const_iterator it = _vgrid.begin(); it != _vgrid.end(); ++it )
	{
		g_texman->DrawSprite(_barTexture, 0, 0x44444444, sx, sy - (_rangeMax - *it) * scale, GetWidth(), -1, 0);
	}

	g_texman->DrawBitmapText(_titleFont, _title, 0x77777777, sx, sy);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

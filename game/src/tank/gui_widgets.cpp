// gui_widgets.cpp

#include "globals.h"

#include "gui_widgets.h"
#include "Level.h"

#include "video/TextureManager.h"


//#include "network/TankClient.h"
//#include "network/TankServer.h"

#include <GuiManager.h>

namespace UI
{
///////////////////////////////////////////////////////////////////////////////
// FPS counter implementation

FpsCounter::FpsCounter(Window *parent, float x, float y, enumAlignText align)
  : Text(parent)
  , _nSprites(0)
  , _nLights(0)
  , _nBatches(0)
{
	SetTimeStep(true);
	Move(x, y);
	SetAlign(align);
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

		sprintf(s, "fps:%04d-%04d-%04d; wnd:%03d",
			int(1.0f / max + 0.5f), 
			int(1.0f / avr + 0.5f), 
			int(1.0f / min + 0.5f),
			-1
		);

		sprintf(s1, "; obj:%u\ntimestep: %4u",
			(unsigned int) g_level->GetList(LIST_objects).size(),
			(unsigned int) g_level->ts_fixed.size()
		);
		strcat(s, s1);

#ifndef NDEBUG
		sprintf(s1, "; %4ugarbage", (unsigned int) g_level->_garbage.size());
		strcat(s, s1);
#endif


		// network statistics
/*		if( g_client )
		{
		//	NetworkStats ns;
		//	g_client->GetStatistics(&ns);
		//	sprintf_s(s1, "\nsent:%uk; recv:%uk pending:%u timebuf:%f", 
		//		ns.bytesSent>>10, ns.bytesRecv>>10, ns.bytesPending, g_level->_timeBuffer);
		//	strcat(s, s1);


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
		}*/

		SetText(s);
	}

	_nSprites = 0;
	_nLights  = 0;
	_nBatches = 0;
}

///////////////////////////////////////////////////////////////////////////////

TimeElapsed::TimeElapsed(Window *parent, float x, float y, enumAlignText align)
  : Text(parent)
{
	SetTimeStep(true);
	Move(x, y);
	SetAlign(align);
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
		int time = (int) g_level->GetTime();

		if( time % 60 < 10 )
			sprintf(text, "%d:0%d", time / 60, time % 60);
		else
			sprintf(text, "%d:%d", time / 60, time % 60);

		SetText(text);
	}
	else
	{
		SetText("--:--");
	}
}

///////////////////////////////////////////////////////////////////////////////

Oscilloscope::Oscilloscope(Window *parent, float x, float y)
  : Window(parent)
  , _barTexture(g_texman->FindSprite("ui/bar"))
  , _titleFont(g_texman->FindSprite("font_small"))
  , _rangeMin(-0.1f)
  , _rangeMax(0.1f)
  , _gridStepX(1)
  , _gridStepY(1)
  , _scale(3)
{
	Move(x, y);
	SetTexture("ui/list", true);
	SetDrawBorder(true);
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

	AutoGrid();
	AutoRange();
}

void Oscilloscope::SetRange(float rmin, float rmax)
{
	_rangeMin = rmin;
	_rangeMax = rmax;
}

void Oscilloscope::SetTitle(const std::string &title)
{
	_title = title;
}

void Oscilloscope::SetGridStep(float stepX, float stepY)
{
	_gridStepY = stepY;
	_gridStepX = stepX;
}

void Oscilloscope::AutoGrid()
{
	assert(!_data.empty());

	float valMin = _data[0];
	float valMax = _data[0];
	for( size_t i = 1; i < _data.size(); ++i )
	{
		valMin = std::min(valMin, _data[i]);
		valMax = std::max(valMax, _data[i]);
	}

	float range = valMax - valMin;
	if( range != 0 )
	{
		float cheight = g_texman->GetCharHeight(_titleFont);
		float count = floor((GetHeight() - cheight) / cheight / 2);
		float dy = range / count;
		if( dy < 1 )
		{
			float p = pow(10.0f, -floor(log10(dy)));
			_gridStepY = ceil(dy * p) / p;
		}
		else if ( dy > 1 )
		{
			float p = pow(10.0f, floor(log10(dy)));
			_gridStepY = ceil(dy / p) * p;
		}
		else
		{
			_gridStepY = 1;
		}
	}
	else
	{
		_gridStepY = 0;
	}
}

void Oscilloscope::AutoRange()
{
	assert(!_data.empty());

	float valMin = _data[0];
	float valMax = _data[0];
	for( size_t i = 1; i < _data.size(); ++i )
	{
		valMin = std::min(valMin, _data[i]);
		valMax = std::max(valMax, _data[i]);
	}

	if( _gridStepY != 0 )
	{
		_rangeMax = ceil(valMax / _gridStepY) * _gridStepY;
		_rangeMin = floor(valMin / _gridStepY) * _gridStepY;
	}
	else
	{
		_rangeMax = valMax;
		_rangeMin = valMin;
	}

	if( _rangeMin == _rangeMax )
	{
		_rangeMin = 0;
		_rangeMax = valMax * 1.5f;
	}
}

void Oscilloscope::DrawChildren(const DrawingContext *dc, float sx, float sy) const
{
	float labelOffset = g_texman->GetCharHeight(_titleFont) / 2;
	sy += labelOffset;

	float scale = (GetHeight() - labelOffset * 2) / (_rangeMin - _rangeMax);
	float center = sy - _rangeMax * scale;
	float dx = sx + GetWidth() - (float) _data.size() * _scale;

	// data
	for( size_t i = 0; i < _data.size(); ++i )
	{
		g_texman->DrawSprite(_barTexture, 0, 0x44444444, (float) i * _scale + dx, center, 2, _data[i] * scale, vec2d(1,0));
	}

	// grid
	if( _gridStepY != 0 )
	{
		int start = int(_rangeMin / _gridStepY);
		int stop = int(_rangeMax / _gridStepY);
		for( int i = start; i <= stop; ++i )
		{
			float y = (float) i * _gridStepY;
			g_texman->DrawSprite(_barTexture, 0, 0x44444444, sx, sy - (_rangeMax - y) * scale, GetWidth(), -1, vec2d(1,0));
			char buf[64];
			sprintf(buf, "%.3g", y);
			float dx = float(6 * strlen(buf)); // FIXME: calc true char width
			dc->DrawBitmapText(sx + GetWidth() - dx, sy - (_rangeMax - y) * scale - labelOffset, _titleFont, 0x77777777, buf);
		}
	}
	else
	{
		dc->DrawSprite(_barTexture, 0, 0x44444444, sx, sy - _rangeMax * scale, GetWidth(), -1, vec2d(1,0));
	}

	dc->DrawBitmapText(sx, sy - labelOffset, _titleFont, 0x77777777, _title);
}

///////////////////////////////////////////////////////////////////////////////
} // end of namespace UI

// end of file

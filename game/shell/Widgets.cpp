#include "Widgets.h"

#include <as/AppState.h>
#include <ctx/GameContext.h>
#include <gc/World.h>
#include <ui/DataSource.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <video/RenderContext.h>
#include <sstream>
#include <iomanip>

FpsCounter::FpsCounter(UI::TimeStepManager &manager, enumAlignText align, AppState &appState)
	: UI::TimeStepping(manager)
	, _nSprites(0)
	, _nLights(0)
	, _nBatches(0)
	, _appState(appState)
{
	SetTimeStep(true);
	SetAlign(align);
}

void FpsCounter::OnTimeStep(const Plat::Input &input, bool focused, float dt)
{
	_totalTime += dt;
	_minDt = std::min(_minDt, dt);
	_maxDt = std::max(_maxDt, dt);
	_totalSteps++;

	if (_totalTime > 0.7f)
	{
		float averageFps = (float) _totalSteps / _totalTime;

		std::ostringstream s;
		s << std::setfill('0');
		s << "fps:" << std::setw(3) << int(1.0f / _maxDt + 0.5f) << '-' << std::setw(3) << int(averageFps + 0.5f) << '-' << std::setw(3) << int(1.0f / _minDt + 0.5f);
		if (GameContextBase *gc = _appState.GetGameContext().get())
		{
			s << std::setfill(' ');
			s << " objects:" << gc->GetWorld().GetList(LIST_objects).size();
			s << "\ntimestep:" << std::setw(6) << std::left << gc->GetWorld().GetList(LIST_timestep).size();
			s << " timeout:" << gc->GetWorld().GetResumableCount();

#ifndef NDEBUG
			s << " " << std::setw(4) << gc->GetWorld()._garbage.size() << "garbage";
#endif
		}


		// network statistics
/*		if( g_client )
		{
		//	NetworkStats ns;
		//	g_client->GetStatistics(&ns);
		//	sprintf_s(s1, "\nsent:%uk; recv:%uk pending:%u timebuf:%f",
		//		ns.bytesSent>>10, ns.bytesRecv>>10, ns.bytesPending, world._timeBuffer);
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

		SetText(std::make_shared<UI::StaticText>(s.str()));

		_minDt = FLT_MAX;
		_maxDt = 0;
		_totalTime = 0;
		_totalSteps = 0;
	}

	_nSprites = 0;
	_nLights  = 0;
	_nBatches = 0;
}

///////////////////////////////////////////////////////////////////////////////

Oscilloscope::Oscilloscope()
  : _rangeMin(-0.1f)
  , _rangeMax(0.1f)
  , _gridStepX(1)
  , _gridStepY(1)
  , _stepSize(2)
{
	SetTexture("ui/list");
	SetDrawBorder(true);
}

void Oscilloscope::Push(TextureManager &texman, float value)
{
	_data.push_back(value);
	size_t size = (size_t) (GetWidth() / _stepSize);
	if( _data.size() > size )
	{
		_data.erase(_data.begin(), _data.begin() + (_data.size() - size));
	}

	AutoGrid(texman);
	AutoRange();
}

void Oscilloscope::SetRange(float rmin, float rmax)
{
	_rangeMin = rmin;
	_rangeMax = rmax;
}

void Oscilloscope::SetTitle(std::string title)
{
	_title = std::move(title);
}

void Oscilloscope::SetGridStep(float stepX, float stepY)
{
	_gridStepY = stepY;
	_gridStepX = stepX;
}

void Oscilloscope::AutoGrid(TextureManager &texman)
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
		float cheight = texman.GetCharHeight(_titleFont.GetTextureId(texman));
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

void Oscilloscope::Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const
{
	UI::Rectangle::Draw(dc, sc, lc, ic, rc, texman, time, hovered);

	float pxLabelOffset = UI::ToPx(texman.GetCharHeight(_titleFont.GetTextureId(texman)) / 2, lc);
	float pxAvailableSpace = lc.GetPixelSize().y - pxLabelOffset * 2;

	float dataScale = pxAvailableSpace / (_rangeMin - _rangeMax);

	size_t texBar = _barTexture.GetTextureId(texman);

	float pxStepSize = UI::ToPx(_stepSize, lc);
	float pxBarWidth = pxStepSize > 2 ? pxStepSize - 1 : pxStepSize;

	float xoffset = lc.GetPixelSize().x - (float)_data.size() * pxStepSize;

	// data
	for( size_t i = 0; i < _data.size(); ++i )
	{
		FRECT rect;
		rect.left = (float)i * pxStepSize + xoffset;
		rect.right = rect.left + pxBarWidth;
		rect.bottom = lc.GetPixelSize().y - pxLabelOffset;
		rect.top = rect.bottom + (_data[i] - _rangeMin) * dataScale;

		rc.DrawSprite(rect, texBar, 0x44444444, 0);
	}

	// grid
	float pxCharWidth = UI::ToPx(texman.GetCharWidth(_titleFont.GetTextureId(texman)), lc);
	if( _gridStepY != 0 )
	{
		int start = int(_rangeMin / _gridStepY);
		int stop = int(_rangeMax / _gridStepY);
		for( int i = start; i <= stop; ++i )
		{
			float y = (float)i * _gridStepY;
			std::ostringstream buf;
			buf << y;
			std::string text = buf.str();
			
			float pxTextWidth = pxCharWidth * (float) text.size();

			rc.DrawSprite(texBar, 0, 0x44444444, vec2d{ pxTextWidth, pxLabelOffset - (_rangeMax - y) * dataScale }, lc.GetPixelSize().x - pxTextWidth, -1, vec2d{ 1, 0 });
			rc.DrawBitmapText(vec2d{ 0, std::floor((y - _rangeMax) * dataScale) }, lc.GetScaleCombined(), _titleFont.GetTextureId(texman), 0x77777777, text);
		}
	}
	else
	{
		rc.DrawSprite(texBar, 0, 0x44444444, vec2d{ 0, pxLabelOffset - _rangeMax * dataScale }, lc.GetPixelSize().x, -1, vec2d{ 1, 0 });
	}

	// title
	rc.DrawBitmapText(vec2d{ std::floor(lc.GetPixelSize().x / 2), 0 }, lc.GetScaleCombined(), _titleFont.GetTextureId(texman), 0x77777777, _title, alignTextCT);
}

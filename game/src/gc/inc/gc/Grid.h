// Grid.h
///////////////////////////////////////////////

#pragma once

#include <math/MyMath.h>

#include <algorithm>
#include <cassert>
#include <vector>

template <class T>
class Grid
{
	T *_data;
	int _cx;
	int _cy;

public:
	Grid()
	  : _data(nullptr)
	  , _cx(0)
	  , _cy(0)
	{
	}

	~Grid()
	{
		delete [] _data;
	}

	void resize(unsigned int cx, unsigned int cy)
	{
		assert(cx > 0 && cy > 0);
		delete [] _data;
		_data = new T[cy*cx];
		_cx = cx;
		_cy = cy;
	}

	inline T& element(unsigned int x, unsigned int y)
	{
		assert(x < (unsigned int) _cx && y < (unsigned int) _cy);
		return _data[_cx*y + x];
	}

	inline const T& element(unsigned int x, unsigned int y) const
	{
		assert(x < (unsigned int) _cx && y < (unsigned int) _cy);
		return _data[_cx*y + x];
	}

	///////////////////////////////////////////////////////////////////////////

	void OverlapRect(std::vector<T*> &receive, const FRECT &rect)
	{
		int xmin = std::max(0, (int) floorf(rect.left - 0.5f));
		int ymin = std::max(0, (int) floorf(rect.top  - 0.5f));
		int xmax = std::min(_cx-1, (int) floorf(rect.right  + 0.5f));
		int ymax = std::min(_cy-1, (int) floorf(rect.bottom + 0.5f));

		for( int y = ymin; y <= ymax; ++y )
		{
			for( int x = xmin; x <= xmax; ++x )
			{
				receive.push_back(&element(x, y));
			}
		}
	}

	void OverlapPoint(std::vector<T*> &receive, const vec2d &pt)
	{
		int xmin = std::min(std::max((int) floorf(pt.x - 0.5f), 0), _cx-1);
		int ymin = std::min(std::max((int) floorf(pt.y - 0.5f), 0), _cy-1);
		int xmax = std::min(std::max((int) floorf(pt.x + 0.5f), 0), _cx-1);
		int ymax = std::min(std::max((int) floorf(pt.y + 0.5f), 0), _cy-1);

		for( int y = ymin; y <= ymax; ++y )
		{
			for( int x = xmin; x <= xmax; ++x )
			{
				receive.push_back(&element(x, y));
			}
		}
	}
};

///////////////////////////////////////////////////////////////////////////////
// end of file

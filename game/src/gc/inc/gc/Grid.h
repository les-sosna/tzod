// Grid.h
///////////////////////////////////////////////

#pragma once

#include <math/MyMath.h>

#include <algorithm>
#include <cassert>
#include <vector>

template <class T>
class Grid final
{
public:
	Grid()
		: _data(nullptr)
		, _bounds()
	{
	}

	~Grid()
	{
		delete [] _data;
	}

	void resize(RectRB bounds)
	{
		assert(WIDTH(bounds) > 0 && HEIGHT(bounds) > 0);
		delete [] _data;
		_data = new T[WIDTH(bounds) * HEIGHT(bounds)];
		_bounds = bounds;
	}

	inline T& element(int x, int y)
	{
		assert(PtInRect(_bounds, x, y));
		return _data[WIDTH(_bounds)*(y - _bounds.top) + x - _bounds.left];
	}

	inline const T& element(int x, int y) const
	{
		assert(PtInRect(_bounds, x, y));
		return _data[WIDTH(_bounds)*(y - _bounds.top) + x - _bounds.left];
	}

	///////////////////////////////////////////////////////////////////////////

	void OverlapRect(std::vector<T*> &receive, const FRECT &rect)
	{
		int xmin = std::max(_bounds.left, (int)std::floor(rect.left - 0.5f));
		int ymin = std::max(_bounds.top, (int)std::floor(rect.top - 0.5f));
		int xmax = std::min(_bounds.right - 1, (int)std::floor(rect.right + 0.5f));
		int ymax = std::min(_bounds.bottom - 1, (int)std::floor(rect.bottom + 0.5f));

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
		int xmin = std::min(std::max((int)std::floor(pt.x - 0.5f), _bounds.left), _bounds.right - 1);
		int ymin = std::min(std::max((int)std::floor(pt.y - 0.5f), _bounds.top), _bounds.bottom - 1);
		int xmax = std::min(std::max((int)std::floor(pt.x + 0.5f), _bounds.left), _bounds.right - 1);
		int ymax = std::min(std::max((int)std::floor(pt.y + 0.5f), _bounds.top), _bounds.bottom - 1);

		for( int y = ymin; y <= ymax; ++y )
		{
			for( int x = xmin; x <= xmax; ++x )
			{
				receive.push_back(&element(x, y));
			}
		}
	}

private:
	T * _data;
	RectRB _bounds;
};

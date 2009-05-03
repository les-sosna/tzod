// Grid.h
///////////////////////////////////////////////

#pragma once

template <class T>
class Grid
{
	T *_data;
	size_t _cx;
	size_t _cy;

public:
	Grid()
	  : _data(NULL)
	  , _cx(0)
	  , _cy(0)
	{
	}

	~Grid()
	{
		delete [] _data;
	}

	void resize(size_t cx, size_t cy)
	{
		delete [] _data;
		_data = new T[cy*cx];
		_cx = cx;
		_cy = cy;
	}

	inline T& element(size_t x, size_t y)
	{
		assert(x < _cx && y < _cy);
		return _data[_cx*y + x];
	}

	///////////////////////////////////////////////////////////////////////////

	void OverlapRect(PtrList<T> &receive, const FRECT &rect)
	{
		int xmin = __max(0, int(floorf(rect.left - 0.5f)));
		int ymin = __max(0, int(floorf(rect.top  - 0.5f)));
		int xmax = __min(g_level->_locationsX-1, int(floorf(rect.right  + 0.5f)));
		int ymax = __min(g_level->_locationsY-1, int(floorf(rect.bottom + 0.5f)));

		for( int y = ymin; y <= ymax; ++y )
		{
			for( int x = xmin; x <= xmax; ++x )
			{
				receive.push_back(&element(x, y));
			}
		}
	}

	void OverlapPoint(PtrList<T> &receive, const vec2d &pt)
	{
		int xmin = __min(__max(int(floorf(pt.x - 0.5f)), 0), g_level->_locationsX-1);
		int ymin = __min(__max(int(floorf(pt.y - 0.5f)), 0), g_level->_locationsX-1);
		int xmax = __min(__max(int(floorf(pt.x + 0.5f)), 0), g_level->_locationsX-1);
		int ymax = __min(__max(int(floorf(pt.y + 0.5f)), 0), g_level->_locationsY-1);

		for( int y = ymin; y <= ymax; ++y )
		{
			for( int x = xmin; x <= xmax; ++x )
			{
				receive.push_back(&element(x, y));
			}
		}
	}



	// broken implementation
	//void OverlapCircle(std::vector<T*> &receive, float cx, float cy, float cr)
	//{
	//	int xmin = __max(0, int(cx - cr));
	//	int ymin = __max(0, int(cy - cr));
	//	int xmax = __min(g_level->_locationsX-1, int(cx + cr));
	//	int ymax = __min(g_level->_locationsY-1, int(cy + cr));

	//	for( int x = xmin; x <= xmax; ++x )
	//	{
	//		for( int y = ymin; y <= ymax; ++y )
	//		{
	//			if( (float) y      <= cy && (float) (y+1)      >= cy &&
	//				(float) x - cr <= cx && (float) (x+1) + cr >= cx )
	//			{
	//				receive.push_back(&element(x, y));
	//				continue;
	//			}

	//			if( (float) x      <= cx && (float) (x+1)      >= cx &&
	//				(float) y - cr <= cy && (float) (y+1) + cr >= cy )
	//			{
	//				receive.push_back(&element(x, y));
	//				continue;
	//			}

	//			/////////////////////////////////////////////

	//			if( ((float)x    -cx)*((float)x    -cx) + ((float)y    -cy)*((float)y    -cy) <= cr*cr ||
	//				((float)(x+1)-cx)*((float)(x+1)-cx) + ((float)y    -cy)*((float)y    -cy) <= cr*cr ||
	//				((float)x    -cx)*((float)x    -cx) + ((float)(y+1)-cy)*((float)(y+1)-cy) <= cr*cr ||
	//				((float)(x+1)-cx)*((float)(x+1)-cx) + ((float)(y+1)-cy)*((float)(y+1)-cy) <= cr*cr )
	//			{
	//				receive.push_back(&element(x, y));
	//			}
	//		}
	//	}
	//}
};

////////////////////////////////////////////////////

struct Location
{
	int x;
	int y;
//	int level;
};

///////////////////////////////////////////////////////////////////////////////
// end of file

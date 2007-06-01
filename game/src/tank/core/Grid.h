// Grid.h
///////////////////////////////////////////////

#pragma once

/////////////////////////////////////////////////////////

template <class grid_type>
class Grid {
	std::vector<grid_type> _data;
	size_t _cx;
	size_t _cy;

public:
	Grid(size_t cx, size_t cy) {
		_cx = cx;
		_cy = cy;
		_data.resize(cy*cx);
	}

	inline grid_type& element(size_t x, size_t y) {
		_ASSERT(x < _cx && y < _cy);
		return _data[y*_cx + x];
	}
};

////////////////////////////////////////////////////

/*        X
     *----*-->
     |0    1
     |
   Y *    *
     |2    3
*/

template <class element_type>
class GridSet
{
	Grid<element_type> *_ppGrids[4];

public:
	void resize(size_t rows, size_t cols) {
		for( size_t i = 0; i < 4; i++ )
		{
			if( _ppGrids[i] )
			{
				delete _ppGrids[i];
			}
			_ppGrids[i] = new Grid<element_type>(rows, cols);
		}
	}

	GridSet() {
		for( size_t i = 0; i < 4; ++i )
			_ppGrids[i] = NULL;
	}
	GridSet(size_t rows, size_t cols) { init(rows, cols); }

	inline Grid<element_type>& operator() (size_t n) {
		_ASSERT(n < 4);
		return *_ppGrids[n];
	}
	inline element_type& operator() (size_t n, size_t x, size_t y) {
		_ASSERT(n < 4);
		return _ppGrids[n]->element(x, y);
	}

	////////////////////////

	// не очищает список receive
//	void OverlapRect(std::vector<element_type*> &receive, const FRECT &rect)
	void OverlapRect(PtrList<element_type> &receive, const FRECT &rect)
	{
		static const float dx[4] = {0,-0.5f, 0,-0.5f};
		static const float dy[4] = {0, 0,-0.5f,-0.5f};
		for( int i = 0; i < 4; ++i )
		{
			int xmin = __max(0, (int) (rect.left + dx[i]));
			int ymin = __max(0, (int) (rect.top  + dx[i]));
			int xmax = __min(g_level->_locations_x-1, (int) (rect.right  + dx[i]));
			int ymax = __min(g_level->_locations_y-1, (int) (rect.bottom + dx[i]));

			for( int x = xmin; x <= xmax; ++x )
			for( int y = ymin; y <= ymax; ++y )
			{
				receive.push_back(&(*this)(i, x, y));
			}
		}
	}

	// не очищает список receive
	void OverlapCircle(std::vector<element_type*> &receive, float cx, float cy, float cr)
	{
		static const float dx[4] = {0,-0.5f, 0,-0.5f};
		static const float dy[4] = {0, 0,-0.5f,-0.5f};
		for( int i = 0; i < 4; ++i )
		{
			float cx_ = cx + dx[i];
			float cy_ = cy + dy[i];

			int xmin = __max(0, (int) (cx_ - cr));
			int ymin = __max(0, (int) (cy_ - cr));
			int xmax = __min(g_level->_locations_x-1, (int) (cx_ + cr));
			int ymax = __min(g_level->_locations_y-1, (int) (cy_ + cr));

			for( int x = xmin; x <= xmax; ++x )
			for( int y = ymin; y <= ymax; ++y )
			{
				if( (float) y      <= cy_ && (float) (y+1)      >= cy_ &&
					(float) x - cr <= cx_ && (float) (x+1) + cr >= cx_ )
				{
					receive.push_back(&(*this)(i, x, y));
					continue;
				}

				if( (float) x      <= cx_ && (float) (x+1)      >= cx_ &&
					(float) y - cr <= cy_ && (float) (y+1) + cr >= cy_ )
				{
					receive.push_back(&(*this)(i, x, y));
					continue;
				}

				/////////////////////////////////////////////

                if( ((float)x    -cx_)*((float)x    -cx_) + ((float)y    -cy_)*((float)y    -cy_) <= cr*cr ||
					((float)(x+1)-cx_)*((float)(x+1)-cx_) + ((float)y    -cy_)*((float)y    -cy_) <= cr*cr ||
					((float)x    -cx_)*((float)x    -cx_) + ((float)(y+1)-cy_)*((float)(y+1)-cy_) <= cr*cr ||
					((float)(x+1)-cx_)*((float)(x+1)-cx_) + ((float)(y+1)-cy_)*((float)(y+1)-cy_) <= cr*cr )
				{
					receive.push_back(&(*this)(i, x, y));
				}
			}
		}
	}

	// не очищает список receive
	// x0 - начальная точка
	// a  - направление и глубина трассировки
	void OverlapLine(std::vector<element_type*> &receive, const vec2d &x0, const vec2d &a)
	{
		static const float  dx[4] = {0,-0.5f, 0,-0.5f};
		static const float  dy[4] = {0, 0,-0.5f,-0.5f};

		const int stepx[2] = {a.x >= 0 ? 1 : -1, 0};
		const int stepy[2] = {0, a.y >= 0 ? 1 : -1};
		const int check[2] = {a.y == 0,   a.x == 0};

		float la = a.Lenght();

		for( int i = 0; i < 4; ++i )
		{
			float cx = x0.x + dx[i];
			float cy = x0.y + dy[i];

			int targx = (int) floor(cx + a.x);
			int targy = (int) floor(cy + a.y);
			int curx  = (int) floor(cx);
			int cury  = (int) floor(cy);

			int xmin = __min(curx, targx);
			int xmax = __max(curx, targx);
			int ymin = __min(cury, targy);
			int ymax = __max(cury, targy);

			while (1)
			{
				if( curx >= 0 && curx < g_level->_locations_x &&
					cury >= 0 && cury < g_level->_locations_y )
				{
					receive.push_back( &(*this)(i, curx, cury) );
				}

				if( curx == targx && cury == targy ) break;

				float d_min;
				int   j_min = 0;
				for( int j = 0; j < 2; ++j )
				{
					int newx = curx + stepx[j];
					int newy = cury + stepy[j];

					if( newx < xmin || newx > xmax || newy < ymin || newy > ymax )
					{
						j_min = 1 - j;
						break;
					}

					float d = fabsf(a.x*((float) newy + 0.5f - cy) -
						a.y*((float) newx + 0.5f - cx)) / la;

					if( 0 == j )
						d_min = d;
					else if( d < d_min || check[j] )
					{
						j_min = j;
						d_min = d;
					}
				}
				curx += stepx[j_min];
				cury += stepy[j_min];
			}
		}
	}
	////////////////////////

	~GridSet()
	{
		for( size_t i = 0; i < 4; ++i )
			delete _ppGrids[i];
	}
};

////////////////////////////////////////////////////

struct Location
{
	int x;
	int y;
	int level;
};

///////////////////////////////////////////////////////////////////////////////
// end of file

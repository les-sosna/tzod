// MyMath.h
#pragma once

#include <cmath>
#include <cassert>
#include <cstdlib>

#define PI    3.141593f
#define PI2   6.283185f
#define PI4   0.785398f

/////////////////////////////////////////////////////////////////////////////
struct FRECT
{
	float left;
	float top;
	float right;
	float bottom;
};

class vec2d
{
public:
	float x, y;
	vec2d() {};

	vec2d(float _x, float _y)
	{
		x = _x;
		y = _y;
	}

	explicit vec2d(float angle)
	{
		x = cosf(angle);
		y = sinf(angle);
	}

public:
	vec2d& operator-=(const vec2d &v)
	{
		x -= v.x;
		y -= v.y;
		return *this;
	}

	vec2d& operator+=(const vec2d &v)
	{
		x += v.x;
		y += v.y;
		return *this;
	}

	vec2d operator + (const vec2d &v) const
	{
		return vec2d(x + v.x, y + v.y);
	}

	vec2d operator - (const vec2d &v) const
	{
		return vec2d(x - v.x, y - v.y);
	}

	vec2d operator - () const
	{
		return vec2d(-x, -y);
	}

	vec2d operator * (float a) const
	{
		return vec2d(x * a, y * a);
	}

	vec2d operator / (float a) const
	{
		return vec2d(x / a, y / a);
	}

	const vec2d& operator *= (float a)
	{
		x *= a;
		y *= a;
		return *this;
	}

	const vec2d& operator /= (float a)
	{
		x /= a;
		y /= a;
		return *this;
	}

	float operator * (const vec2d &v) const
	{
		return x * v.x + y * v.y;  // dot product
	}

	friend vec2d operator * (float a, const vec2d &v)
	{
		return vec2d(v.x * a, v.y * a);
	}

	bool operator ==(const vec2d &v) const
	{
		return v.x == x && v.y == y;
	}

	bool operator !=(const vec2d &v) const
	{
		return v.x != x || v.y != y;
	}


public:
	float sqr() const
	{
		return x*x + y*y;
	}

	float len() const
	{
		return sqrt(x*x + y*y);
	}

	float Angle() const // angle to the X axis
	{
		float a = atan2(y, x);
		return (a < 0) ? (a + PI2) : a;
	}

	vec2d& Normalize()
	{
		float len = sqrt(x*x + y*y);
		if( len < 1e-7 )
		{
			Zero();
		}
		else
		{
			x /= len;
			y /= len;
		}
		return *this;
	}

	const vec2d& Set(float _x, float _y)
	{
		x = _x;
		y = _y;
		return *this;
	}

	void Zero()
	{
		x = 0;
		y = 0;
	}
};

struct Rect
{
	int left;
	int top;
	int right;
	int bottom;
};

struct Point
{
    int x, y;
};

class CRect : public Rect
{
public:
    CRect(int l, int t, int r, int b)
    {
        left = l;
        top = t;
        right = r;
        bottom = b;
    }
};

inline int WIDTH(const Rect &rect) { return rect.right - rect.left; }
inline int HEIGHT(const Rect &rect) { return rect.bottom - rect.top; }


inline vec2d Vec2dAddDirection(const vec2d &a, const vec2d &b)
{
	assert(abs(a.sqr() - 1) < 1e-5);
	assert(abs(b.sqr() - 1) < 1e-5);
	return vec2d(a.x*b.x - a.y*b.y, a.y*b.x + a.x*b.y);
}

inline vec2d Vec2dSubDirection(const vec2d &a, const vec2d &b)
{
	assert(abs(a.sqr() - 1) < 1e-5);
	assert(abs(b.sqr() - 1) < 1e-5);
	return vec2d(a.x*b.x + a.y*b.y, a.y*b.x - a.x*b.y);
}

inline float Vec2dCross(const vec2d &a, const vec2d &b)
{
	return a.x*b.y - a.y*b.x;
}

inline float Vec2dDot(const vec2d &a, const vec2d &b)
{
	return a.x*b.x + a.y*b.y;
}

inline bool PtInFRect(const FRECT &rect, const vec2d &pt)
{
	return rect.left <= pt.x && pt.x < rect.right &&
		rect.top <= pt.y && pt.y < rect.bottom;
}

inline bool PtInRect(const Rect &rect, const Point &pt)
{
    return rect.left <= pt.x && pt.x < rect.right &&
        rect.top <= pt.y && pt.y < rect.bottom;
}

inline void RectToFRect(FRECT *lpfrt, const Rect *lprt)
{
	lpfrt->left   = (float) lprt->left;
	lpfrt->top    = (float) lprt->top;
	lpfrt->right  = (float) lprt->right;
	lpfrt->bottom = (float) lprt->bottom;
}

inline void FRectToRect(Rect *lprt, const FRECT *lpfrt)
{
	lprt->left   = (int) lpfrt->left;
	lprt->top    = (int) lpfrt->top;
	lprt->right  = (int) lpfrt->right;
	lprt->bottom = (int) lpfrt->bottom;
}

inline void OffsetFRect(FRECT *lpfrt, float x, float y)
{
	lpfrt->left   += x;
	lpfrt->top    += y;
	lpfrt->right  += x;
	lpfrt->bottom += y;
}

inline void OffsetFRect(FRECT *lpfrt, const vec2d &x)
{
	lpfrt->left   += x.x;
	lpfrt->top    += x.y;
	lpfrt->right  += x.x;
	lpfrt->bottom += x.y;
}

// generates a pseudo random number in range [0, max)
inline float frand(float max)
{
	return (float) rand() / RAND_MAX * max;
}

// generates a pseudo random vector of the specified length
inline vec2d vrand(float len)
{
	return vec2d(frand(PI2)) * len;
}


///////////////////////////////////////////////////////////////////////////////
// end of file

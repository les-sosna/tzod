#pragma once

#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdlib>

#define PI    3.141593f
#define PI2   6.283185f
#define PI4   0.785398f

namespace math {

class vec2d
{
public:
	float x, y;

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
		return vec2d{ x + v.x, y + v.y };
	}

	vec2d operator - (const vec2d &v) const
	{
		return vec2d{ x - v.x, y - v.y };
	}

	vec2d operator - () const
	{
		return vec2d{ -x, -y };
	}

	vec2d operator * (float a) const
	{
		return vec2d{ x * a, y * a };
	}

	vec2d operator * (const vec2d &v) const
	{
		return vec2d{ x * v.x, y * v.y };
	}

	vec2d operator / (float a) const
	{
		return vec2d{ x / a, y / a };
	}

	vec2d operator / (const vec2d &v) const
	{
		return vec2d{ x / v.x, y / v.y };
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

	friend vec2d operator * (float a, const vec2d &v)
	{
		return vec2d{ v.x * a, v.y * a };
	}

	bool operator ==(const vec2d &v) const
	{
		return v.x == x && v.y == y;
	}

	bool operator !=(const vec2d &v) const
	{
		return v.x != x || v.y != y;
	}


	float sqr() const
	{
		return x*x + y*y;
	}

	float len() const
	{
		return sqrt(x*x + y*y);
	}

	bool IsZero() const
	{
		return x == 0 && y == 0;
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
			x = 0;
			y = 0;
		}
		else
		{
			x /= len;
			y /= len;
		}
		return *this;
	}

	vec2d Norm() const
	{
		vec2d result = *this;
		return result.Normalize();
	}
};

struct FRECT
{
	float left;
	float top;
	float right;
	float bottom;

	FRECT operator *(float m) const
	{
		return FRECT{ left * m, top * m, right * m, bottom * m };
	}
};

inline FRECT MakeRectRB(vec2d lt, vec2d rb) { return FRECT{ lt.x, lt.y, rb.x, rb.y }; }
inline FRECT MakeRectWH(vec2d lt, vec2d size) { return FRECT{ lt.x, lt.y, lt.x + size.x, lt.y + size.y }; }

inline float WIDTH(const FRECT &rect) { return rect.right - rect.left; }
inline float HEIGHT(const FRECT &rect) { return rect.bottom - rect.top; }
inline vec2d Size(const FRECT &rect) { return { WIDTH(rect), HEIGHT(rect) }; }
inline vec2d Center(const FRECT &rect) { return vec2d{ rect.left + rect.right, rect.top + rect.bottom} / 2; }

struct RectRB
{
	int left;
	int top;
	int right;
	int bottom;

	RectRB operator *(int m) const
	{
		return RectRB{ left * m, top * m, right * m, bottom * m };
	}
};

inline int WIDTH(const RectRB &rect) { return rect.right - rect.left; }
inline int HEIGHT(const RectRB &rect) { return rect.bottom - rect.top; }

inline vec2d Vec2dConstrain(const vec2d &vec, const FRECT &rect)
{
	return vec2d{ std::max(rect.left, std::min(vec.x, rect.right)), std::max(rect.top, std::min(vec.y, rect.bottom)) };
}

inline vec2d Vec2dFloor(const vec2d &vec)
{
	return{ std::floor(vec.x), std::floor(vec.y) };
}

inline vec2d Vec2dDirection(float angle)
{
	return{ cosf(angle), sinf(angle) };
}

inline vec2d Vec2dAddDirection(const vec2d &a, const vec2d &b)
{
	assert(std::abs(a.sqr() - 1) < 1e-5);
	assert(std::abs(b.sqr() - 1) < 1e-5);
	return{ a.x*b.x - a.y*b.y, a.y*b.x + a.x*b.y };
}

inline vec2d Vec2dSubDirection(const vec2d &a, const vec2d &b)
{
	assert(std::abs(a.sqr() - 1) < 1e-5);
	assert(std::abs(b.sqr() - 1) < 1e-5);
	return{ a.x*b.x + a.y*b.y, a.y*b.x - a.x*b.y };
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

inline bool PtInRect(const RectRB &rect, int x, int y)
{
	return rect.left <= x && x < rect.right &&
		rect.top <= y && y < rect.bottom;
}

inline FRECT RectToFRect(const RectRB &rect)
{
	return FRECT{
		(float)rect.left,
		(float)rect.top,
		(float)rect.right,
		(float)rect.bottom
	};
}

inline void FRectToRect(RectRB *lprt, const FRECT *lpfrt)
{
	lprt->left   = (int) lpfrt->left;
	lprt->top    = (int) lpfrt->top;
	lprt->right  = (int) lpfrt->right;
	lprt->bottom = (int) lpfrt->bottom;
}

inline FRECT RectOffset(const FRECT &rect, vec2d offset)
{
	return FRECT{rect.left + offset.x, rect.top + offset.y, rect.right + offset.x, rect.bottom + offset.y};
}

// generates a pseudo random number in range [0, max)
inline float frand(float max)
{
	return (float) rand() / RAND_MAX * max;
}

// generates a pseudo random vector of the specified length
inline vec2d vrand(float len)
{
	return Vec2dDirection(frand(PI2)) * len;
}

} // namespace math

using namespace math;

// MyMath.h
#pragma once

#include "constants.h"

/////////////////////////////////////////////////////////////////////////////

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

	vec2d(float angle)
	{
		x = cosf(angle);
		y = sinf(angle);
	}

	vec2d(const vec2d &rVector)
	{
		x = rVector.x;
		y = rVector.y;
	}

public:
	const vec2d& operator =(const vec2d &rVector)
	{
		x = rVector.x;
		y = rVector.y;
		return *this;
	}

	vec2d& operator-=(const vec2d &rVector)
	{
		x -= rVector.x;
		y -= rVector.y;
		return *this;
	}

	vec2d& operator+=(const vec2d &rVector)
	{
		x += rVector.x;
		y += rVector.y;
		return *this;
	}

	vec2d  operator + (const vec2d &rVector) const
	{
		return vec2d(x + rVector.x, y + rVector.y);
	}

	vec2d  operator - (const vec2d &rVector) const
	{
		return vec2d(x - rVector.x, y - rVector.y);
	}

	vec2d  operator - () const
	{
		return vec2d(-x, -y);
	}

	vec2d  operator * (float a) const
	{
		return vec2d(x * a, y * a);
	}

	vec2d  operator / (float a) const
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

	float operator*(const vec2d &rVector) const
	{
		return x*rVector.x + y*rVector.y;  // dot product
	}

public:
	float Square() const
	{
		return x*x + y*y;
	}

	float Length() const
	{
		return sqrtf(x*x + y*y);
	}

	float Angle() const // угол к оси X
	{
		float a = atan2f(y, x);
		return (a < 0) ? (a + PI2) : a;
	}

	vec2d& Normalize() // приведение к единичной длине
	{
		float len = sqrtf(x*x + y*y);
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

///////////////////////////////////////////////////////////////////////////////
// end of file

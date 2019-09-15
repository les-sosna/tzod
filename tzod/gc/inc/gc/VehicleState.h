#pragma once
#include <math/MyMath.h>

struct VehicleState
{
	vec2d steering;
	float gas;         // from -1 to 1
	float weaponAngle; // relative to the body

	union
	{
		struct
		{
			bool rotateWeapon : 1;
			bool attack : 1;
			bool pickup : 1;
			bool light : 1;
		};
		int flags;
	};
};


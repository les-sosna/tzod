#pragma once

struct VehicleState
{
	float bodyAngle;
	float weaponAngle; // relative to the body
	union
	{
		struct
		{
			bool rotateBody : 1;
			bool rotateWeapon : 1;
			bool moveForward : 1;
			bool moveBack : 1;
			bool attack : 1;
			bool pickup : 1;
			bool light : 1;
		};
		int flags;
	};
};


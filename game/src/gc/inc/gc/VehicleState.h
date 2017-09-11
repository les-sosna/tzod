#pragma once

struct VehicleState
{
	float bodyAngle;
	float weaponAngle; // relative to the body
	float gas;         // from -1 to 1

	union
	{
		struct
		{
			bool rotateBody : 1;
			bool rotateWeapon : 1;
			bool attack : 1;
			bool pickup : 1;
			bool light : 1;
		};
		int flags;
	};
};


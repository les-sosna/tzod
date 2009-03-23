// ControlPacket.cpp

#include "stdafx.h"
#include "ControlPacket.h"

VARIANT_IMPLEMENT_TYPE(ControlPacket) RAW;
VARIANT_IMPLEMENT_TYPE(ControlPacketVector) STD_VECTOR;

///////////////////////////////////////////////////////////////////////////////

ControlPacket::ControlPacket()
{
	ZeroMemory(this, sizeof(*this));
}

void ControlPacket::fromvs(const VehicleState &vs)
{
	wControlState = 0;
	weap = 0;
	body = 0;

	wControlState |= STATE_MOVEFORWARD * (false != vs._bState_MoveForward);
	wControlState |= STATE_MOVEBACK    * (false != vs._bState_MoveBack);
	wControlState |= STATE_FIRE        * (false != vs._bState_Fire);
	wControlState |= STATE_ALLOWDROP   * (false != vs._bState_AllowDrop);
	wControlState |= STATE_ENABLELIGHT * (false != vs._bLight);

	if( vs._bExplicitBody )
	{
		body = (unsigned short) (int(vs._fBodyAngle / PI2 * 65536.0f + 0.5f) & 0xffff);
		wControlState |= MODE_EXPLICITBODY;
	}
	else
	{
		wControlState |= STATE_ROTATELEFT  * (false != vs._bState_RotateLeft);
		wControlState |= STATE_ROTATERIGHT * (false != vs._bState_RotateRight);
	}

	if( vs._bExplicitTower )
	{
		weap = (unsigned short) (int(vs._fTowerAngle / PI2 * 65536.0f + 0.5f) & 0xffff);
		wControlState |= MODE_EXPLICITTOWER;
	}
	else
	{
		wControlState |= STATE_TOWERLEFT   * (false != vs._bState_TowerLeft);
		wControlState |= STATE_TOWERRIGHT  * (false != vs._bState_TowerRight);
		wControlState |= STATE_TOWERCENTER * (false != vs._bState_TowerCenter);
	}
}

void ControlPacket::tovs(VehicleState &vs) const
{
	ZeroMemory(&vs, sizeof(VehicleState));

	vs._bState_MoveForward = (0 != (wControlState & STATE_MOVEFORWARD));
	vs._bState_MoveBack    = (0 != (wControlState & STATE_MOVEBACK));
	vs._bState_Fire        = (0 != (wControlState & STATE_FIRE));
	vs._bState_AllowDrop   = (0 != (wControlState & STATE_ALLOWDROP));

	vs._bLight         = (0 != (wControlState & STATE_ENABLELIGHT));

	vs._bExplicitBody  = (0 != (wControlState & MODE_EXPLICITBODY));
	vs._bExplicitTower = (0 != (wControlState & MODE_EXPLICITTOWER));

	if( vs._bExplicitBody)
	{
		vs._fBodyAngle = (float) body / 65536.0f * PI2;
	}
	else
	{
		vs._bState_RotateLeft  = (0 != (wControlState & STATE_ROTATELEFT));
		vs._bState_RotateRight = (0 != (wControlState & STATE_ROTATERIGHT));
	}

	if( vs._bExplicitTower)
		vs._fTowerAngle = (float) weap / 65536.0f * PI2;
	else
	{
		vs._bState_TowerLeft   = (0 != (wControlState & STATE_TOWERLEFT));
		vs._bState_TowerRight  = (0 != (wControlState & STATE_TOWERRIGHT));
		vs._bState_TowerCenter = (0 != (wControlState & STATE_TOWERCENTER));
	}
}


// end of file

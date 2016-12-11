#include "ShootingAgent.h"
#include <gc/SaveFile.h>
#include <gc/Vehicle.h>
#include <gc/WeaponBase.h>
#include <gc/World.h>

void ShootingAgent::Serialize(SaveFile &f)
{
	f.Serialize(_currentOffset);
	f.Serialize(_desiredOffset);
}

void ShootingAgent::TowerTo(const GC_Vehicle &vehicle, VehicleState *pState, const vec2d &x, bool bFire, const AIWEAPSETTINGS *ws)
{
	assert(vehicle.GetWeapon());

	vec2d tmp = x - vehicle.GetPos();
	if (tmp.x && tmp.y)
	{
		tmp.Normalize();
		tmp = Vec2dAddDirection(tmp, Vec2dDirection(_currentOffset));
		float cosDiff = Vec2dDot(tmp, vehicle.GetWeapon()->GetDirection());
		pState->_bState_Fire = bFire && cosDiff >= ws->fMaxAttackAngleCos;
		pState->_bExplicitTower = true;
		pState->_fTowerAngle = Vec2dSubDirection(tmp, vehicle.GetDirection()).Angle() - vehicle.GetSpinup();
		assert(!std::isnan(pState->_fTowerAngle) && std::isfinite(pState->_fTowerAngle));
	}
	else
	{
		pState->_bState_Fire = bFire;
		pState->_bExplicitTower = false;
		pState->_fTowerAngle = 0;
		pState->_bState_TowerLeft = false;
		pState->_bState_TowerRight = false;
		pState->_bState_TowerCenter = false;
	}
}

// calculates the position of a fake target for more accurate shooting
void ShootingAgent::CalcOutstrip(const World &world, vec2d origin, const GC_Vehicle &target, float Vp, vec2d &fake)
{
	float Vt = target._lv.len();
	if (Vt < Vp)
	{
		float c = target.GetDirection().x;
		float s = target.GetDirection().y;

		float x = (target.GetPos().x - origin.x) * c +
			(target.GetPos().y - origin.y) * s;
		float y = (target.GetPos().y - origin.y) * c -
			(target.GetPos().x - origin.x) * s;

		float fx = x + Vt * (x * Vt + sqrt(Vp*Vp * (y*y + x*x) - Vt*Vt * y*y)) / (Vp*Vp - Vt*Vt);

		fake.x = origin.x + fx * c - y * s;
		fake.y = origin.y + fx * s + y * c;

		fake = Vec2dConstrain(fake, world._bounds);
	}
	else
	{
		fake = origin;
	}
}

void ShootingAgent::AttackTarget(World &world, const GC_Vehicle &myVehicle, const GC_RigidBodyStatic &target, VehicleState *pVehState)
{
	// select a _currentOffset to reduce shooting accuracy
	const float acc_speed = 0.4f; // angular velocity of a fake target
	if (auto targetVehicle = PtrDynCast<GC_Vehicle>(&target))
	{
		float len = fabsf(_desiredOffset - _currentOffset);
		if (acc_speed * dt >= len)
		{
			_currentOffset = _desiredOffset;

			static float d_array[5] = { 0.186f, 0.132f, 0.09f, 0.05f, 0.00f };

			float d = d_array[_difficulty];

			if (_difficulty > 2)
			{
				d = d_array[_difficulty] * fabs(targetVehicle->_lv.len()) / targetVehicle->GetMaxSpeed();
			}

			_desiredOffset = (d > 0) ? (world.net_frand(d) - d * 0.5f) : 0;
		}
		else
		{
			_currentOffset += (_desiredOffset - _currentOffset) * dt * acc_speed / len;
		}
	}
	else
	{
		_desiredOffset = 0;
		_currentOffset = 0;
	}

	assert(myVehicle.GetWeapon());

	vec2d fake = target.GetPos();
	GC_Vehicle *enemy = PtrDynCast<GC_Vehicle>(&target);
	if (ws->bNeedOutstrip && _difficulty > 1 && enemy)
	{
		CalcOutstrip(world, myVehicle.GetPos(), *enemy, ws->fProjectileSpeed, fake);
	}

	float len = (target.GetPos() - myVehicle.GetPos()).len();
	TowerTo(vehicle, pVehState, fake, len > ws->fAttackRadius_crit, ws);
}

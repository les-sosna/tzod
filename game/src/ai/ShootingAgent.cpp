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

static void TowerTo(const GC_Vehicle &vehicle, VehicleState *pState, const vec2d &at, bool bFire, const AIWEAPSETTINGS &weapSettings, float offsetAngle)
{
	assert(vehicle.GetWeapon());

	vec2d direction = at - vehicle.GetPos();
	direction.Normalize();
	if (!direction.IsZero())
	{
		direction = Vec2dAddDirection(direction, Vec2dDirection(offsetAngle));
		float cosDiff = Vec2dDot(direction, vehicle.GetWeapon()->GetDirection());
		pState->_bState_Fire = bFire && cosDiff >= weapSettings.fMaxAttackAngleCos;
		pState->_bExplicitTower = true;
		pState->_fTowerAngle = Vec2dSubDirection(direction, vehicle.GetDirection()).Angle() - vehicle.GetSpinup();
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

void ShootingAgent::AttackTarget(World &world, const GC_Vehicle &myVehicle, const GC_RigidBodyStatic &target, float dt, VehicleState &outVehicleState)
{
	auto targetAsVehicle = PtrDynCast<const GC_Vehicle>(&target);

	// select a _currentOffset to reduce shooting accuracy
	if (targetAsVehicle)
	{
		const float acc_speed = 0.4f; // angular velocity of a fake target
		float len = fabsf(_desiredOffset - _currentOffset);
		if (acc_speed * dt >= len)
		{
			_currentOffset = _desiredOffset;

			static float d_array[5] = { 0.186f, 0.132f, 0.09f, 0.05f, 0.00f };

			float d = d_array[_accuracy];

			if (_accuracy > 2)
			{
				d = d_array[_accuracy] * fabs(targetAsVehicle->_lv.len()) / targetAsVehicle->GetMaxSpeed();
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

	AIWEAPSETTINGS weapSettings;
	assert(myVehicle.GetWeapon());
	myVehicle.GetWeapon()->SetupAI(&weapSettings);

	vec2d fake = target.GetPos();
	if (weapSettings.bNeedOutstrip && _accuracy > 1 && targetAsVehicle)
	{
		world.CalcOutstrip(myVehicle.GetPos(), weapSettings.fProjectileSpeed, targetAsVehicle->GetPos(), targetAsVehicle->_lv, fake);
	}

	float len = (target.GetPos() - myVehicle.GetPos()).len();
	TowerTo(myVehicle, &outVehicleState, fake, len > weapSettings.fAttackRadius_crit, weapSettings, _currentOffset);
}

void ShootingAgent::SetAccuracy(int accuracy)
{
	assert(accuracy >= 0 && accuracy < 5);
	_accuracy = accuracy;
}

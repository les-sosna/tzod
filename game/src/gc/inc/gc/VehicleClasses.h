#pragma once

#include <memory>
#include <string>

struct VehicleClass
{
	std::string displayName;

	float health;
	float percussion;
	float fragility;

	float length;
	float width;

	float m, i;

	float _Nx;      // dry friction factor X
	float _Ny;      // dry friction factor Y
	float _Nw;      // angular dry friction factor

	float _Mx;      // viscous friction factor X
	float _My;      // viscous friction factor Y
	float _Mw;      // angular viscous friction factor

	float enginePower;
	float rotatePower;

	float maxRotSpeed;
	float maxLinSpeed;
};

std::shared_ptr<const VehicleClass> GetVehicleClass(const char *className);
const char* GetVehicleClassName(unsigned int index);

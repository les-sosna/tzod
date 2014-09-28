#include "VehicleClasses.h"
#include <string.h>

static const char* s_vehicleClassNames[] =
{
	"default",
	"heavy"
};

static VehicleClass s_vehicleClasses[] = {
	{
		"Default",  // displayName
		
		400.0f,     // health
		1.0f,       // percussion
		1.0f,       // fragility
		
		37.5f,      // length
		37.0f,      // width
		
		1.0f,       // m - mass
		700.0f,     // i - inertia
		
		450.0f,     // _Nx - dry friction factor X
		5000.0f,    // _Ny - dry friction factor Y
		28.0f,      // _Nw - angular dry friction factor
		
		2.0,        // _Mx - viscous friction factor X
		2.5,        // _My - viscous friction factor Y
		0,          // _Mw - angular viscous friction factor
		
		900.0f,     // enginePower
		40.0f,      // rotatePower
		
		3.4f,       // maxRotSpeed
		200.0f      // maxLinSpeed
	},
	{
		"Heavy",    // displayName
		
		600.0f,     // health
		1.5f,       // percussion
		0.8f,       // fragility
		
		40.0f,      // length
		40.0f,      // width
		
		2.0f,       // m - mass
		1000.0f,    // i - inertia
		
		450.0f,     // _Nx - dry friction factor X
		5000.0f,    // _Ny - dry friction factor Y
		27.0f,      // _Nw - angular dry friction factor
		
		2.0,        // _Mx - viscous friction factor X
		2.5,        // _My - viscous friction factor Y
		9,          // _Mw - angular viscous friction factor
		
		1800.0f,    // enginePower
		48.0f,      // rotatePower
		
		3.5f,       // maxRotSpeed
		180.0f      // maxLinSpeed
	}
};

std::shared_ptr<const VehicleClass> GetVehicleClass(const char *className)
{
	for (int i = 0; i != sizeof(s_vehicleClasses) / sizeof(s_vehicleClasses[0]); ++i)
	{
		if (!strcmp(className, s_vehicleClassNames[i]))
			return std::make_shared<VehicleClass>(s_vehicleClasses[i]);
	}
	return nullptr;
}

const char* GetVehicleClassName(unsigned int index)
{
	if (index < sizeof(s_vehicleClasses) / sizeof(s_vehicleClasses[0]))
		return s_vehicleClassNames[index];
	return nullptr;
}

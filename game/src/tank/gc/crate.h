// crate.h

#include "RigidBodyDinamic.h"


class GC_Crate : public GC_RigidBodyDynamic
{
	DECLARE_SELF_REGISTRATION(GC_Crate);

public:
	GC_Crate(World &world);
	GC_Crate(FromFile);
	~GC_Crate();

	virtual void OnDestroy(World &world);

	virtual float GetDefaultHealth() const { return 50; }
	virtual unsigned char GetPassability() const { return 0; }

	// GC_2dSprite
	virtual enumZOrder GetZ() const { return Z_WALLS; }
};


// end of file

// crate.h

#include "RigidBodyDinamic.h"


class GC_Crate : public GC_RigidBodyDynamic
{
	DECLARE_SELF_REGISTRATION(GC_Crate);

public:
	GC_Crate(World &world, float x, float y);
	GC_Crate(FromFile);
	~GC_Crate();

	virtual void OnDestroy(World &world);

	virtual float GetDefaultHealth() const { return 50; }
	virtual unsigned char GetPassability() const { return 0; }
};


// end of file

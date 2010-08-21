// crate.h

#include "RigidBodyDinamic.h"


class GC_Crate : public GC_RigidBodyDynamic
{
	DECLARE_SELF_REGISTRATION(GC_Crate);

public:
	GC_Crate(float x, float y);
	GC_Crate(FromFile);
	~GC_Crate();

	virtual void OnDestroy();

	virtual float GetDefaultHealth() const { return 50; }
	virtual unsigned char GetPassability() const { return 1; }
};


// end of file

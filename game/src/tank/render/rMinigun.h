#include "rWeaponBase.h"
#include <stddef.h> // size_t

class TextureManager;

class R_WeaponMinigun : public R_WeaponBase
{
public:
	R_WeaponMinigun(TextureManager &tm);
	
	// ObjectView
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId1;
	size_t _texId2;
};

class R_Crosshair2 : public ObjectView
{
public:
	R_Crosshair2(TextureManager &tm);
	
	// ObjectView
	virtual enumZOrder GetZ(const World &world, const GC_Actor &actor) const { return Z_VEHICLE_LABEL; }
	virtual void Draw(const World &world, const GC_Actor &actor, DrawingContext &dc) const override;
	
private:
	size_t _texId;
};

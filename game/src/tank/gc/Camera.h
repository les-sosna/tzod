// Camera.h

#pragma once

#include "Actor.h"
#include "core/Rotator.h"

// forward declarations
class GC_Player;


class GC_Camera : public GC_Actor
{
	DECLARE_SELF_REGISTRATION(GC_Camera);
	MemberOfGlobalList<LIST_cameras> _memberOf;

private:
	vec2d  _target;
	float  _time_shake;
	float  _time_seed;
	float  _rotatorAngle;

	Rotator _rotator;

	RECT    _viewport;
	float   _zoom;
	SafePtr<GC_Player>  _player;

public:
	explicit GC_Camera(const SafePtr<GC_Player> &player);
	GC_Camera(FromFile);

	float GetAngle() const { return _rotatorAngle; }
	void GetWorld(FRECT &outWorld) const;
	void GetScreen(RECT &vp) const;
	float GetZoom() const { return _zoom; }
	const SafePtr<GC_Player>& GetPlayer() const { assert(_player); return _player; }

	static void UpdateLayout();
	static bool GetWorldMousePos(vec2d &pos);

	void Shake(float level);
	float GetShake() const { return _time_shake; }

	// overrides
	virtual void Kill();
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFloat(float dt);

	// message handlers
	void OnDetach(GC_Object *sender, void *param);
};

// end of file

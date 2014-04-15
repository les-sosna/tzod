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

	Rect    _viewport;
	float   _zoom;
	ObjPtr<GC_Player>  _player;

public:
	explicit GC_Camera(GC_Player *player);
	GC_Camera(FromFile);
	virtual ~GC_Camera();

	float GetAngle() const { return _rotatorAngle; }
	void GetWorld(FRECT &outWorld) const;
	void GetScreen(Rect &vp) const;
	float GetZoom() const { return _zoom; }
	GC_Player* GetPlayer() const { assert(_player); return _player; }

	static void UpdateLayout();
	static bool GetWorldMousePos(vec2d &pos, bool editorMode);

	void Shake(float level);
	float GetShake() const { return _time_shake; }

	// message handlers
	void OnDetach(GC_Object *sender, void *param);

	// GC_Object
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFloat(float dt);
};

// end of file

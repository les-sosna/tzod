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
	float  _angle_current;
	DWORD  _dwTimeX;
	DWORD  _dwTimeY;
	float  _dt;
	bool   _active;

	Rotator _rotator;


	//--------------------------------
public:
	RECT    _viewport;
	float   _zoom;
	SafePtr<GC_Player>  _player;

	//--------------------------------
public:
	GC_Camera(SafePtr<GC_Player> &player);
	GC_Camera(FromFile);

	void Select();         // применение трансформации, выбор камеры как текущей
	void Activate(bool bActivate);  // неактивная камера не отображается на экране
	bool IsActive() const { return _active && !IsKilled(); }
	void GetViewport(RECT &vp) const;

	static void SwitchEditor();
	static void UpdateLayout(); // пересчет координат viewports
	static bool GetWorldMousePos(vec2d &pos);

	void Shake(float level);
	float GetShake() const { return _time_shake; }

	// overrides
	virtual void Kill();
	virtual bool IsSaved() const { return _player != NULL; }
	virtual void Serialize(SaveFile &f);
	virtual void TimeStepFloat(float dt);
	virtual void EndFrame();

	// message handlers
	void OnDetach(GC_Object *sender, void *param);
};

// end of file

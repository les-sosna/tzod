#pragma once
#include <math/MyMath.h>

class GC_Player;
class World;

class Camera
{
public:
	Camera(vec2d pos, GC_Player &player);

	void CameraTimeStep(World &world, float dt, float scale);

	void SetViewport(RectRB viewport) { _viewport = viewport; }
	RectRB GetViewport() const { return _viewport; }

	vec2d GetCameraPos() const;
	GC_Player& GetPlayer() const { return _player; }
	float GetShake() const { return _time_shake; }
	void Shake(float level);

private:
	GC_Player &_player;
	vec2d _pos;
	vec2d _target;
	float _time_shake;
	float _time_seed;
	RectRB _viewport;
};

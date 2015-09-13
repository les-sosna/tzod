#pragma once

#include <math/MyMath.h>

class GC_Vehicle;
class World;
class SaveFile;

class Camera
{
public:
    Camera(vec2d pos, unsigned int index);

	void CameraTimeStep(World &world, const GC_Vehicle *vehicle, float dt);

	void SetViewport(RectRB viewport) { _viewport = viewport; }
	RectRB GetViewport() const { return _viewport; }

	vec2d GetCameraPos() const;
    unsigned int GetIndex() const { return _index; }
	float GetShake() const { return _time_shake; }
    void Shake(float level);

	void Serialize(World &world, SaveFile &f);

private:
    unsigned int _index;
    vec2d _pos;
    vec2d _target;
    float _time_shake;
    float _time_seed;
	RectRB _viewport;
};

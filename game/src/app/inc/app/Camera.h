#pragma once

#include <math/MyMath.h>

class GC_Vehicle;
class World;
class SaveFile;

class Camera
{
public:
    explicit Camera(vec2d pos);

	void CameraTimeStep(World &world, const GC_Vehicle *vehicle, float dt, vec2d viewSize);

	vec2d GetCameraPos() const;

	void Shake(float level);
	float GetShake() const { return _time_shake; }

	void Serialize(World &world, SaveFile &f);
    
private:
    vec2d _pos;
    vec2d _target;
    float _time_shake;
    float _time_seed;
};

#pragma once

#include <math/MyMath.h>

namespace Plat
{
	struct Input;
}

class DefaultCamera
{
public:
	explicit DefaultCamera(vec2d pos);

	void MoveTo(vec2d newEyeWorldPos);
	void Move(vec2d worldOffset, const FRECT &worldBounds);
	void ZoomIn();
	void ZoomOut();
	void HandleMovement(const Plat::Input &input, const FRECT &worldBounds, float dt);
	float GetZoom() const { return _zoom; }
	vec2d GetEye() const { return _worldPos; }

private:
	int _zoomLevel = 4;
	float _zoom = 1;
	vec2d _worldPos{};
	vec2d _targetPos{};
	vec2d _speed{};
	bool _movingToTarget = false;
};

#pragma once

#include <math/MyMath.h>

namespace UI
{
	struct IInput;
}

class DefaultCamera
{
public:
	explicit DefaultCamera(vec2d pos);

	void MoveTo(vec2d newEyeWorldPos);
	void Move(vec2d offset, const FRECT &worldBounds);
	void ZoomIn();
	void ZoomOut();
	void HandleMovement(UI::IInput &input, const FRECT &worldBounds, float dt);
	float GetZoom() const { return _zoom; }
	vec2d GetEye() const { return _pos; }

private:
	int _zoomLevel = 4;
	float _zoom = 1;
	vec2d _pos{};
	vec2d _targetPos{};
	vec2d _speed{};
	bool _movingToTarget = false;
};

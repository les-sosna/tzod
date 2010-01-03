// DefaultCamera.h

#pragma once

class DefaultCamera
{
public:
	DefaultCamera();

	void HandleMovement(float worldWidth, float worldHeight, float screenWidth, float screenHeight);
	float GetZoom() const { return _zoom; }
	float GetPosX() const { return _pos.x; }
	float GetPosY() const { return _pos.y; }

private:
	float _zoom;
	float _dt;
	vec2d _pos;
	DWORD _dwTimeX;
	DWORD _dwTimeY;
};

// end of file

#pragma once
#include <vector>

namespace math
{
	struct RectRB;
}

class AtlasPacker
{
public:
	void ExtendCanvas(int dx, int dy);
	bool PlaceRect(int width, int height, math::RectRB& result);

private:
	struct Segment
	{
		int x, y;
		int length;
	};
	std::vector<Segment> _segments;
	int _canvasHeight = 0;
};

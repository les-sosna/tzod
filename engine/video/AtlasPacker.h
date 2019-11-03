#pragma once
#include <vector>

class AtlasPacker
{
public:
	void ExtendCanvas(int dx, int dy);
	bool PlaceRect(int width, int height, int &outX, int &outY);
	int GetContentHeight() const;

private:
	struct Segment
	{
		int x, y;
		int length;
	};
	std::vector<Segment> _segments;
	int _canvasHeight = 0;
};

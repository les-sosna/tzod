#include "AtlasPacker.h"
#include <math/MyMath.h>

void AtlasPacker::ExtendCanvas(int dx, int dy)
{
	Segment s;
	s.x = _segments.empty() ? 0 : _segments.back().x + _segments.back().length;
	s.y = 0;
	s.length = dx;
	_segments.push_back(s);
	_canvasHeight += dy;
}

bool AtlasPacker::PlaceRect(int width, int height, RectRB& result)
{
	// search for first available range that is closest to the top
	auto overlapFirst = _segments.end();
	auto overlapLast = _segments.end();
	int bestTop = 0x7fffffff;
	int canvasWidth = _segments.back().x + _segments.back().length;

	auto end = _segments.end();
	for (auto it = _segments.begin(); it != end; ++it)
	{
		// stop if no more space left
		if (it->x + width > canvasWidth)
			break;
		// skip if not better or does not fit
		if (it->y >= bestTop || it->y + height > _canvasHeight)
			continue;

		int availableWidth = 0;
		int candidateRangeMaxTop = 0;
		auto candidateRangeFirst = it;
		for (auto candidateRangeLast = it; candidateRangeLast != end; ++candidateRangeLast)
		{
			if (candidateRangeMaxTop <= candidateRangeLast->y)
			{
				candidateRangeMaxTop = candidateRangeLast->y;
				it = candidateRangeLast; // no reason to restart search from same or worse top

				// new top may not fit
				if (candidateRangeMaxTop + height > _canvasHeight)
					break;
			}
			availableWidth += candidateRangeLast->length;
			if (availableWidth >= width)
			{
				const int improvementThreshold = 8; // somewhat reduces staggering, may be 0
				if (bestTop > candidateRangeMaxTop + improvementThreshold)
				{
					bestTop = candidateRangeMaxTop;
					overlapFirst = candidateRangeFirst;
					overlapLast = candidateRangeLast;
				}
				break;
			}
		}
	}

	if (overlapFirst != _segments.end())
	{
		assert(bestTop + height <= _canvasHeight);
		result.left = overlapFirst->x;
		result.top = bestTop;
		result.right = result.left + width;
		result.bottom = result.top + height;

		if (overlapLast->x + overlapLast->length == result.right)
		{
			// replace the entire range with reused first segment
			overlapFirst->y = result.bottom;
			if (overlapFirst != overlapLast) // erase the rest if there is more than one
			{
				overlapFirst->length = width;
				++overlapFirst; // keep first
				++overlapLast;
				_segments.erase(overlapFirst, overlapLast);
			}
		}
		else // the range is slightly longer
		{
			Segment s;
			s.x = result.right;
			s.y = overlapFirst->y;
			s.length = overlapFirst->length - width;

			// always keep and reuse first segment in range
			overlapFirst->y = result.bottom;
			overlapFirst->length = width;

			if (overlapFirst != overlapLast)
			{
				// keep first, reuse last, erase what's in the middle
				overlapLast->length -= result.right - overlapLast->x;
				overlapLast->x = result.right;
				++overlapFirst;
				_segments.erase(overlapFirst, overlapLast);
			}
			else
			{
				// split the first (and only) segment in range
				++overlapFirst;
				_segments.insert(overlapFirst, s);
			}
		}

		return true;
	}

	return false;
}


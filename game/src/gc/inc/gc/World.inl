#include "RigidBody.h"
#include "WorldCfg.h"
#include <cmath>

template<class SelectorType>
void World::RayTrace(const Grid<ObjectList> &list, SelectorType &s) const
{
	//
	// overlap line
	//

	vec2d begin(s.GetCenter() - s.GetDirection()/2), end(s.GetCenter() + s.GetDirection()/2), delta(s.GetDirection());
	begin /= LOCATION_SIZE;
	end   /= LOCATION_SIZE;
	delta /= LOCATION_SIZE;

	const int halfBeginX = int(std::floor(begin.x - 0.5f));
	const int halfBeginY = int(std::floor(begin.y - 0.5f));

	const int halfEndX = int(std::floor(end.x - 0.5f));
	const int halfEndY = int(std::floor(end.y - 0.5f));

	static const int jitX[4] = {0,1,0,1};
	static const int jitY[4] = {0,0,1,1};

	const int stepx = delta.x > 0 ? 2 : -2;
	const int stepy = delta.y > 0 ? 2 : -2;

	const float p = delta.y * (begin.x - 0.5f) - delta.x * (begin.y - 0.5f);
	const float tx = p - delta.y * (float) stepx;
	const float ty = p + delta.x * (float) stepy;

	for( int i = 0; i < 4; i++ )
	{
		int cx = halfBeginX + jitX[i];
		int cy = halfBeginY + jitY[i];

		int count = (abs(cx-halfEndX - (cx<halfEndX)) / 2) + (abs(cy-halfEndY - (cy<halfEndY)) / 2);
		assert(count >= 0);

		do
		{
			// check current cell
			if( PtInRect(_locationBounds, cx, cy) )
			{
				const ObjectList &tmp_list = list.element(cx, cy);
				for( ObjectList::id_type it = tmp_list.begin(); it != tmp_list.end(); it = tmp_list.next(it) )
				{
					GC_RigidBodyStatic *object = static_cast<GC_RigidBodyStatic *>(tmp_list.at(it));
					if( object->GetTrace0() )
					{
						continue;
					}

					float hitEnter, hitExit;
					vec2d hitNorm;
					if( object->IntersectWithLine(s.GetCenter(), s.GetDirection(), hitNorm, hitEnter, hitExit) )
					{
						assert(!std::isnan(hitEnter) && std::isfinite(hitEnter));
						assert(!std::isnan(hitExit) && std::isfinite(hitExit));
						assert(!std::isnan(hitNorm.x) && std::isfinite(hitNorm.x));
						assert(!std::isnan(hitNorm.y) && std::isfinite(hitNorm.y));
#ifndef NDEBUG
//						for( int i = 0; i < 4; ++i )
//						{
//							DbgLine(object->GetVertex(i), object->GetVertex((i+1)&3));
//						}
#endif
						if( s.Select(object, hitNorm, hitEnter, hitExit) )
						{
							return;
						}
					}
				}
			}

			// step to the next cell
			float t = delta.x * (float) cy - delta.y * (float) cx;
			if(std::fabs(t + tx) < std::fabs(t + ty) )
				cx += stepx;
			else
				cy += stepy;
		} while( count-- );
	}
}

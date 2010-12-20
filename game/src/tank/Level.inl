// Level.inl - inline functions for Level.h

#include "gc/RigidBody.h"


template<class SelectorType>
void Level::RayTrace(Grid<ObjectList> &list, SelectorType &s) const
{
	//
	// overlap line
	//

	vec2d begin(s.GetCenter() - s.GetDirection()/2), end(s.GetCenter() + s.GetDirection()/2), delta(s.GetDirection());
	begin /= LOCATION_SIZE;
	end   /= LOCATION_SIZE;
	delta /= LOCATION_SIZE;

	const int halfBeginX = int(floor(begin.x - 0.5f));
	const int halfBeginY = int(floor(begin.y - 0.5f));

	const int halfEndX = int(floor(end.x - 0.5f));
	const int halfEndY = int(floor(end.y - 0.5f));

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

		int count = (abs(cx-halfEndX - (cx<halfEndX))>>1) + (abs(cy-halfEndY - (cy<halfEndY))>>1);
		assert(count >= 0);

		do
		{
			// check current cell
			if( cx >= 0 && cx < _locationsX && cy >= 0 && cy < _locationsY )
			{
				const ObjectList &tmp_list = list.element(cx, cy);
				for( ObjectList::iterator it = tmp_list.begin(); it != tmp_list.end(); ++it )
				{
					GC_RigidBodyStatic *object = static_cast<GC_RigidBodyStatic *>(*it);
					if( object->CheckFlags(GC_FLAG_RBSTATIC_PHANTOM|GC_FLAG_RBSTATIC_TRACE0) )
					{
						continue;
					}

					float hitEnter, hitExit;
					vec2d hitNorm;
					if( object->CollideWithLine(s.GetCenter(), s.GetDirection(), hitNorm, hitEnter, hitExit) )
					{
						assert(!_isnan(hitEnter) && _finite(hitEnter));
						assert(!_isnan(hitExit) && _finite(hitExit));
						assert(!_isnan(hitNorm.x) && _finite(hitNorm.x));
						assert(!_isnan(hitNorm.y) && _finite(hitNorm.y));
#ifndef NDEBUG
						for( int i = 0; i < 4; ++i )
						{
							g_level->DbgLine(object->GetVertex(i), object->GetVertex((i+1)&3));
						}
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
			if( fabs(t + tx) < fabs(t + ty) )
				cx += stepx;
			else
				cy += stepy;
		} while( count-- );
	}
}


// end of file

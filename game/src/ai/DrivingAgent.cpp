#include "DrivingAgent.h"
#include <gc/Field.h>
#include <gc/SaveFile.h>
#include <gc/Turrets.h>
#include <gc/Vehicle.h>
#include <gc/VehicleState.h>
#include <gc/WeaponBase.h>
#include <gc/World.h>
#include <gc/WorldCfg.h>

#include <functional>

static void CatmullRom(const vec2d &p1, const vec2d &p2, const vec2d &p3, const vec2d &p4, vec2d &out, float s)
{
	float s2 = s * s;
	float s3 = s2 * s;
	out = (p1*(2 * s2 - s3 - s) + p2*(3 * s3 - 5 * s2 + 2) + p3*(4 * s2 - 3 * s3 + s) + p4*(s3 - s2)) * 0.5f;
}

#define NODE_RADIUS         32.0f
#define MIN_PATH_ANGLE       0.8f

#define GRID_ALIGN(x, sz)    ((x)-(x)/(sz)*(sz)<(sz)/2)?((x)/(sz)):((x)/(sz)+1)


void DrivingAgent::Serialize(SaveFile &f)
{
	f.Serialize(_backTime);
	f.Serialize(_attackFriendlyTurrets);

	if (f.loading())
	{
		size_t size;
		f.Serialize(size);
		while (size--)
		{
			_attackList.push_back(AttackListType::value_type());
			f.Serialize(_attackList.back());
		}
	}
	else
	{
		size_t size = _attackList.size();
		f.Serialize(size);
		for (AttackListType::iterator it = _attackList.begin(); _attackList.end() != it; ++it)
		{
			f.Serialize(*it);
		}
	}
}


// check the cell's passability taking into account current weapon settings
inline static bool CheckCell(const FieldCell &cell, bool hasWeapon)
{
	return (hasWeapon && 0xFF != cell.Properties()) || (!hasWeapon && 0 == cell.Properties());
}

static float EstimatePathLength(RefFieldCell begin, RefFieldCell end)
{
	int dx = abs(end.x - begin.x);
	int dy = abs(end.y - begin.y);
	return (float)std::max(dx, dy) + (float)std::min(dx, dy) * 0.4142f;
}

float DrivingAgent::CreatePath(World &world, vec2d from, vec2d to, int team, float max_depth, bool bTest, const AIWEAPSETTINGS *ws)
{
	if (!PtInFRect(world.GetBounds(), to))
	{
		return -1;
	}

	Field::NewSession();
	Field &field = *world._field;

	struct OpenListNode
	{
		RefFieldCell cellRef;
		float totalEstimate;

		bool operator<(OpenListNode other) const
		{
			return totalEstimate > other.totalEstimate;
		}
	};
	std::priority_queue<OpenListNode> open;

	RefFieldCell startRef = { (int)std::floor(from.x / WORLD_BLOCK_SIZE + 0.5f), (int)std::floor(from.y / WORLD_BLOCK_SIZE + 0.5f) };
	RefFieldCell endRef = { (int)std::floor(to.x / WORLD_BLOCK_SIZE + 0.5f), (int)std::floor(to.y / WORLD_BLOCK_SIZE + 0.5f) };

	FieldCell &start = field(startRef.x, startRef.y);

	if( !CheckCell(start, !!ws) )
		return -1;

	start.Check();
	start._before = 0;
	start._stepX = 0;
	start._stepY = 0;

	open.push({ startRef, EstimatePathLength(startRef, endRef) });

	while( !open.empty() )
	{
		OpenListNode currentNode = open.top();
		open.pop();

		FieldCell &current = field(currentNode.cellRef.x, currentNode.cellRef.y);


		// neighbor nodes check order
		//    4 | 0 | 6
		//   ---+---+---
		//    2 | n | 3
		//   ---+---+---
		//    7 | 1 | 5
		//                          0  1  2  3  4  5  6  7
		constexpr int per_x[8] = {  0, 0,-1, 1,-1, 1, 1,-1 };  // node x offset
		constexpr int per_y[8] = { -1, 1, 0, 0,-1, 1,-1, 1 };  // node y offset
		constexpr float dist [8] = {
			1.0f, 1.0f, 1.0f, 1.0f,
			1.4142f, 1.4142f, 1.4142f, 1.4142f };             // path cost

		// for diagonal checks
		//                           4     5     6     7
//		static int check_diag[] = { 0,2,  1,3,  3,0,  2,1 };

		for( int i = 0; i < 8; ++i )
		{
/*			if( i > 3 ) // check diagonal passability
			if( !CheckCell(field(currentRef.x + per_x[check_diag[(i-4)*2  ]],
			                     currentRef.y + per_y[check_diag[(i-4)*2  ]]), !!ws) ||
			    !CheckCell(field(currentRef.x + per_x[check_diag[(i-4)*2+1]],
			                     currentRef.y + per_y[check_diag[(i-4)*2+1]]), !!ws) )
			{
				continue;
			}*/


			RefFieldCell nextRef = { currentNode.cellRef.x + per_x[i], currentNode.cellRef.y + per_y[i] };
			FieldCell &next = field(nextRef.x, nextRef.y);
			if( CheckCell(next, !!ws) )
			{
				// increase path cost when travel through the walls
				float dist_mult = 1;
				if( 1 == next.Properties() )
					dist_mult = ws->fDistanceMultipler;

				float nextBefore = current.Before() + dist[i] * dist_mult;

#if 0 // too expensive
				// penalty for turns
				if (cn._stepX || cn._stepY) // TODO: use initial vehicle direction
				{
					float stepSqr = float(cn._stepX*cn._stepX + cn._stepY*cn._stepY);
					float c = float(cn._stepX * per_x[i] + cn._stepY * per_y[i]) / (dist[i] * std::sqrt(stepSqr));
					if (c < 0.5f) // 0 is 90 deg. turn
						before += 1;
					else if (c < 0.9f) // 0.707 is 45 deg. turn
						before += 0.2f;
				}
#endif

				// never visited or found a better path to node
				if( !next.IsChecked() || nextBefore < next._before)
				{
					next.Check();
					next._before = nextBefore;
					next._stepX = per_x[i];
					next._stepY = per_y[i];

					float nextTotal = nextBefore + EstimatePathLength(nextRef, endRef);
					if (nextTotal < max_depth)
					{
						// may add same cell ref with a different total
						open.push({ nextRef, nextTotal });
					}
				}
			}
		}
	}

	if( field(endRef.x, endRef.y).IsChecked() )
	{
		float distance = field(endRef.x, endRef.y).Before();

		if( !bTest )
		{
			ClearPath();

			RefFieldCell currentRef = endRef;
			FieldCell *current = &field(currentRef.x, currentRef.y);

			_path.push_back(to);

			while( current->_stepX || current->_stepY )
			{
				// trace back
				currentRef.x -= current->_stepX;
				currentRef.y -= current->_stepY;
				current = &field(currentRef.x, currentRef.y);

				_path.push_back(vec2d{ (float)(currentRef.x * WORLD_BLOCK_SIZE), (float)(currentRef.y * WORLD_BLOCK_SIZE) });

				for( unsigned int i = 0; i < current->GetObjectsCount(); ++i )
				{
					assert(ws);
					assert(current->Properties() > 0);

					GC_RigidBodyStatic *object = current->GetObject(i);

					if( team && !_attackFriendlyTurrets)
					{
						auto turret = dynamic_cast<GC_Turret*>(object);
						if( turret && (turret->GetTeam() == team) )
						{
							continue;
						}
					}
					_attackList.push_front(object);
				}
			}

			std::reverse(begin(_path), end(_path));

			assert(startRef == currentRef);
		}

		return distance;
	}

	// path not found
	return -1;
}

void DrivingAgent::SmoothPath()
{
//	if( _path.size() < 4 )
		return;

	vec2d vn[4];
	std::vector<vec2d>::iterator it[4], tmp;

	// smooth angles
	if( _path.size() > 4 )
	{
		it[1] = _path.begin();
		it[0] = it[1]++;
		while( it[1] != _path.end() )
		{
			vec2d new_node = (*it[0] + *it[1]) * 0.5f;
			_path.insert(it[1], new_node);
			if( it[0] != _path.begin() )
				_path.erase(it[0]);
			it[0] = it[1]++;
		}
	}


	// spline interpolation
	tmp = _path.begin();
	for( int i = 0; i < 4; ++i )
	{
		it[i] = tmp++;
		vn[i] = *it[i];
	}

	for(;;)
	{
		vec2d new_node;

		for( int i = 1; i < 4; ++i )
		{
			CatmullRom(vn[0], vn[1], vn[2], vn[3], new_node, (float) i / 4.0f);
			_path.insert(it[2], new_node);
		}

		for( int i = 0; i < 3; ++i )
		{
			it[i] = it[i+1];
			vn[i] = vn[i+1];
		}

		if( ++it[3] == _path.end() )
			break;

		vn[3] = *it[3];
	}
}

// returns:
//   * closest path node
//   * projection of 'pos' to the path
//   * offset
std::vector<vec2d>::const_iterator DrivingAgent::FindNearPathNode(const vec2d &pos, vec2d *projection, float *offset) const
{
	assert(!_path.empty());

	// find closest node
	auto it = _path.begin(), result = it;
	float rr_min = (*it - pos).sqr();
	while( ++it != _path.end() )
	{
		float rr = (*it - pos).sqr();
		if( rr <= rr_min )
		{
			result = it;
			rr_min = rr;
		}
	}
	assert(_path.end() != result);

	if( projection )
	{
		vec2d dev = pos - *result;

		float prevL = -1;
		float nextL = -1;

		vec2d nextDir;
		vec2d prevDir;

		float nextDir2;
		float nextDot;

		float prevDir2;
		float prevDot;

		auto prevIt = result;
		if( _path.begin() != result )
		{
			--prevIt;
			prevDir  = *prevIt - *result;
			prevDir2 = prevDir.sqr();
			prevDot  = Vec2dDot(prevDir, dev);
			prevL    = prevDot / sqrtf(prevDir2);
		}

		auto nextIt = result; ++nextIt;
		if( _path.end() != nextIt )
		{
			nextDir  = *nextIt - *result;
			nextDir2 = nextDir.sqr();
			nextDot  = Vec2dDot(nextDir, dev);
			nextL    = nextDot / sqrtf(nextDir2);
		}

		if( prevL > 0 && prevL > nextL )
		{
			vec2d d = prevDir * (prevDot / prevDir2);
			*projection = *result + d;
			if( offset )
			{
				*offset = d.len();
			}
		}
		else
		if( nextL > 0 && nextL > prevL )
		{
			vec2d d = nextDir * (nextDot / nextDir2);
			*projection = *result + d;
			if( offset )
			{
				*offset = -d.len();
			}
		}
		else
		{
			*projection = *result;
			if( offset )
			{
				*offset = 0;
			}
		}
	}

	return result;
}

void DrivingAgent::ClearPath()
{
	_lastProgressTime = 0;
	_pathProgress = -1;
	_path.clear();
	_attackList.clear();
}

static void RotateTo(const GC_Vehicle &vehicle, VehicleState *outState, const vec2d &x, bool moveForvard, bool moveBack)
{
	assert(!std::isnan(x.x) && !std::isnan(x.y));
	assert(std::isfinite(x.x) && std::isfinite(x.y));

	vec2d newDirection = x - vehicle.GetPos();
	newDirection.Normalize();

	float cosDiff = Vec2dDot(newDirection, vehicle.GetDirection());
	float minDiff = std::cos(MIN_PATH_ANGLE);

	if (moveForvard)
	{
		outState->gas = cosDiff > minDiff ? 1.f : 0.f;
	}
	else if (moveBack)
	{
		outState->gas = cosDiff > minDiff ? -1.f : 0.f;
	}
	outState->steering = newDirection;
}

void DrivingAgent::ComputeState(World &world, const GC_Vehicle &vehicle, float dt, VehicleState &vs)
{
	// clean the attack list
	_attackList.remove_if([](auto &arg){ return !arg; });

	vec2d arrivalPoint = {};

	vec2d brake = vehicle.GetBrakingLength();
//	float brake_len = brake.len();
	float brakeSqr = brake.sqr();

	vec2d currentDir = vehicle.GetDirection();
	vec2d currentPos = vehicle.GetPos();
//	vec2d predictedPos = GetVehicle()->GetPos() + brake;

	if (!_path.empty())
	{
//		vec2d predictedProj;
//		auto predictedNodeIt = FindNearPathNode(predictedPos, &predictedProj, nullptr);

		vec2d currentProj;
		float offset;
		auto currentNodeIt = FindNearPathNode(currentPos, &currentProj, &offset);

		int newProgress = static_cast<int>(std::distance(cbegin(_path), currentNodeIt));
		if (newProgress > _pathProgress)
		{
			_pathProgress = newProgress;
			_lastProgressTime = world.GetTime();
		}

		float desiredProjOffsetLen = vehicle.GetMaxBrakingLength() * 2;
		vec2d desiredProjOffset;

		auto it = currentNodeIt;
		if (offset > 0)
		{
			vec2d d = *it;
			--it;
			d -= *it;
			offset -= d.len();
		}
		offset += std::min((currentPos - currentProj).len(), desiredProjOffsetLen);
		for (;;)
		{
			vec2d d = *it;
			if (++it == _path.end())
			{
				desiredProjOffset = d;
				break;
			}
			d -= *it;
			float len = d.len();
			if (offset + len < desiredProjOffsetLen)
			{
				offset += len;
			}
			else
			{
				float ratio = 1 - (desiredProjOffsetLen - offset) / len;
				desiredProjOffset = *it + d * ratio;
				break;
			}
		}

		if (++currentNodeIt == _path.end() && (currentProj - currentPos).sqr() < WORLD_BLOCK_SIZE*WORLD_BLOCK_SIZE / 16)
		{
			// end of path
			ClearPath();
		}
		//else
		//{
		//	if( (predictedPos - predictedProj).sqr() < WORLD_BLOCK_SIZE*WORLD_BLOCK_SIZE && brake_len > 1 )
		//	{
		//		arrivalPoint = predictedProj + brake;
		//	}
		//	else
		//	{
		arrivalPoint = desiredProjOffset;
		//	}
		//}

	}


	/*
	while( !_path.empty() )
	{
	float desired = _path.size()>1 ? (_pickupCurrent ?
	_pickupCurrent->GetRadius() : 30.0f) : (float) WORLD_BLOCK_SIZE / 2;
	float current = (GetVehicle()->GetPos() - _path.front().coord).len();
	if( current > desired )
	{
	break;
	}
	_path.pop_front();
	}

	if( !_path.empty() )
	destPoint = _path.front().coord;
	*/

	vs.rotateWeapon = true;
	vs.weaponAngle = 0;


	//
	// avoid obstacles
	//

	if (brake.sqr() > 0)
	{
		float min_d = -1;
		vec2d min_hit, min_norm;

		vec2d angle[] = { Vec2dDirection(PI / 4), Vec2dDirection(0), Vec2dDirection(-PI / 4) };
		float len[] = { 1,2,1 };
		for (int i = 0; i < 3; ++i)
		{
			vec2d tmp = Vec2dAddDirection(vehicle.GetDirection(), vec2d(angle[i]));

			vec2d x0 = vehicle.GetPos() + tmp * vehicle.GetRadius();
			vec2d a = brake * len[i];

			vec2d hit, norm;
			GC_Object *o = world.TraceNearest(
				world.grid_rigid_s,
				&vehicle,
				x0,
				a,
				&hit,
				&norm
			);

			if (o)
			{
				if (1 == i && (hit - currentPos).len() < vehicle.GetRadius())
				{
					_backTime = 0.5f;
				}

				float d = (hit - x0).sqr();
				if (min_d < 0 || d < min_d)
				{
					min_d = d;
					min_hit = hit;
					min_norm = norm;
				}
			}
		}

		if (min_d > 0)
		{
			vec2d hit_dir = currentDir; // (min_hit - currentPos).Normalize();
			min_norm = (min_norm - hit_dir * Vec2dDot(min_norm, hit_dir)).Normalize() + min_norm;
			min_norm.Normalize();
			min_norm *= 1.4142f;// sqrt(2)
			arrivalPoint = min_hit + min_norm * vehicle.GetRadius();
//			DbgLine(min_hit, arrivalPoint, 0xff0000ff);
		}
	}

	vec2d d = _stayAwayFrom - arrivalPoint;
	float d_len = d.len();
	if (d_len < _stayAwayRadius)
	{
		if (d_len < 1)
		{
			d = -vehicle.GetDirection();
			d_len = 1;
		}
		arrivalPoint = _stayAwayFrom - d.Norm() * _stayAwayRadius;
	}

	//
	// follow the path
	//

	float dst = (currentPos - arrivalPoint).sqr();
	if (dst > WORLD_BLOCK_SIZE*WORLD_BLOCK_SIZE / 16)
	{
		if (_backTime > 0)
		{
			_lastProgressTime = world.GetTime();
			vs.gas = -1;
		}
		else
		{
			RotateTo(vehicle, &vs, arrivalPoint, dst > brakeSqr, false);
		}
	}

	_backTime -= dt;

	if (_backTime <= 0 && world.GetTime() - _lastProgressTime > 0.6f)
	{
		_backTime = world.net_frand(0.5f);
	}
}

void DrivingAgent::StayAway(vec2d fromCenter, float radius)
{
	_stayAwayFrom = fromCenter;
	_stayAwayRadius = radius;
}

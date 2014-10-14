#include "Explosion.h"
#include "Camera.h"
#include "Light.h"
#include "particles.h"
#include "Player.h"
#include "RigidBody.h"
#include "Sound.h"
#include "Vehicle.h"
#include "World.h"
#include "Macros.h"

#include "SaveFile.h"

IMPLEMENT_SELF_REGISTRATION(GC_Explosion)
{
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_Explosion, LIST_timestep)

GC_Explosion::GC_Explosion(World &world, GC_Player *owner, float duration)
  : _boomOK(false)
  , _owner(owner)
  , _light(new GC_Light(world, GC_Light::LIGHT_POINT))
  , _damage(1)
  , _radius(32)
  , _time(0)
  , _time_life(duration)
  , _time_boom(0)
{
    _light->Register(world);
}

GC_Explosion::GC_Explosion(FromFile)
{
}

GC_Explosion::~GC_Explosion()
{
}

void GC_Explosion::SetRadius(float radius)
{
	_radius = radius;
	_light->SetRadius(radius * 5);
}

void GC_Explosion::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);
	f.Serialize(_boomOK);
	f.Serialize(_damage);
	f.Serialize(_radius);
	f.Serialize(_time);
	f.Serialize(_time_boom);
	f.Serialize(_time_life);
	f.Serialize(_light);
	f.Serialize(_owner);
}

float GC_Explosion::CheckDamage(FIELD_TYPE &field, float dst_x, float dst_y, float max_distance)
{
	int x0 = int(GetPos().x / CELL_SIZE);
	int y0 = int(GetPos().y / CELL_SIZE);
	int x1 = int(dst_x   / CELL_SIZE);
	int y1 = int(dst_y   / CELL_SIZE);

	std::deque<FieldNode *> open;
	open.push_front(&field[coord(x0, y0)]);
	open.front()->checked  = true;
	open.front()->distance = 0;
	open.front()->x = x0;
	open.front()->y = y0;


	while( !open.empty() )
	{
		FieldNode *node = open.back();
		open.pop_back();

		if( x1 == node->x && y1 == node->y )
		{
			// a path was found
			assert(node->GetRealDistance() <= max_distance);
			return node->GetRealDistance();
		}



		//
		// check neighbors
		//
		//  4 | 0  | 6
		// ---+----+---
		//  2 |node| 3
		// ---+----+---
		//  7 | 1  | 5      //   0  1  2  3  4  5  6  7
		static int per_x[8] = {  0, 0,-1, 1,-1, 1, 1,-1 };
		static int per_y[8] = { -1, 1, 0, 0,-1, 1,-1, 1 };
		static int dist [8] = { 12,12,12,12,17,17,17,17 }; // path cost

		static int check_diag[] = { 0,2,  1,3,  3,0,  2,1 };


		for( int i = 0; i < 8; ++i )
		{
			if( i > 3 )
			if( !field[coord(node->x + per_x[check_diag[(i-4)*2  ]],
			                 node->y + per_y[check_diag[(i-4)*2  ]])].open &&
			    !field[coord(node->x + per_x[check_diag[(i-4)*2+1]],
			                 node->y + per_y[check_diag[(i-4)*2+1]])].open )
			{
				continue;
			}

			coord coord_(node->x + per_x[i], node->y + per_y[i]);
			FieldNode *next = &field[coord_];
			next->x = coord_.x;
			next->y = coord_.y;

			if( next->open )
			{
				if( !next->checked )
				{
					next->checked  = true;
					next->parent   = node;
					next->distance = node->distance + dist[i];
					if( next->GetRealDistance() <= max_distance)
						open.push_front(next);
				}
				else if( next->distance > node->distance + dist[i] )
				{
					next->parent   = node;
					next->distance = node->distance + dist[i];
					if( next->GetRealDistance() <= max_distance )
						open.push_front(next);
				}
			}
		}
	}

	return -1;
}

void GC_Explosion::Boom(World &world, float radius, float damage)
{
	FOREACH( world.GetList(LIST_cameras), GC_Camera, pCamera )
	{
		if( !pCamera->GetPlayer() ) continue;
		if( pCamera->GetPlayer()->GetVehicle() )
		{
			float level = 0.5f * (radius - (GetPos() -
				pCamera->GetPlayer()->GetVehicle()->GetPos()).len() * 0.3f) / radius;
			//--------------------------
			if( level > 0 )
				pCamera->Shake(level);
		}
	}

	///////////////////////////////////////////////////////////

	FIELD_TYPE field;

	FieldNode node;
	node.open = false;

	//
	// get a list of locations which are affected by the explosion
	//
    std::vector<ObjectList*> receive;
	FRECT rt = {GetPos().x - radius, GetPos().y - radius, GetPos().x + radius, GetPos().y + radius};
	rt.left   /= LOCATION_SIZE;
	rt.top    /= LOCATION_SIZE;
	rt.right  /= LOCATION_SIZE;
	rt.bottom /= LOCATION_SIZE;
	world.grid_rigid_s.OverlapRect(receive, rt);

	//
	// prepare the field for tracing
	//
	for( auto it = receive.begin(); it != receive.end(); ++it )
	{
		ObjectList::id_type cdit = (*it)->begin();
		while( (*it)->end() != cdit )
		{
			GC_RigidBodyStatic *pDamObject = (GC_RigidBodyStatic *) (*it)->at(cdit);
			cdit = (*it)->next(cdit);

			if( GC_Wall_Concrete::GetTypeStatic() == pDamObject->GetType() )
			{
				node.x = int(pDamObject->GetPos().x / CELL_SIZE);
				node.y = int(pDamObject->GetPos().y / CELL_SIZE);
				field[coord(node.x, node.y)] = node;
			}
		}
	}

	//
	// trace to the nearest objects
	//

	bool bNeedClean = false;
	for( auto it = receive.begin(); it != receive.end(); ++it )
	{
        (*it)->for_each([&](ObjectList::id_type, GC_Object *o)
		{
            auto pDamObject = static_cast<GC_RigidBodyStatic*>(o);
			vec2d dir = pDamObject->GetPos() - GetPos();
			float d = dir.len();

			if( d <= radius)
			{
				GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) world.TraceNearest(
					world.grid_rigid_s, NULL, GetPos(), dir);

				if( object && object != pDamObject )
				{
					if( bNeedClean )
					{
						FIELD_TYPE::iterator fIt = field.begin();
						while (fIt != field.end())
							(fIt++)->second.checked = false;
					}
					d = CheckDamage(field, pDamObject->GetPos().x, pDamObject->GetPos().y, radius);
					bNeedClean = true;
				}

				if( d >= 0 )
				{
					float dam = std::max(0.0f, damage * (1 - d / radius));
					assert(dam >= 0);
					if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(pDamObject) )
					{
						if( d > 1e-5 )
						{
							dyn->ApplyImpulse(dir * (dam / d), dyn->GetPos());
						}
					}
					DamageDesc dd;
					dd.damage = dam;
					dd.hit = GetPos();
					dd.from = _owner;
                    pDamObject->TakeDamage(world, dd);
				}
			}
		});
	}

	_owner = NULL;
	_boomOK = true;
}

void GC_Explosion::MoveTo(World &world, const vec2d &pos)
{
	_light->MoveTo(world, pos);
	GC_Actor::MoveTo(world, pos);
}

void GC_Explosion::TimeStep(World &world, float dt)
{
	GC_Actor::TimeStep(world, dt);

	_time += dt;
	if( _time >= _time_boom && !_boomOK )
		Boom(world, _radius, _damage);

	if( _time >= _time_life )
	{
		if( _time >= _time_life * 1.5f )
		{
			Kill(world);
			return;
		}
	}

	_light->SetIntensity(1.0f - powf(_time / (_time_life * 1.5f - _time_boom), 6));
}

void GC_Explosion::Kill(World &world)
{
	SAFE_KILL(world, _light);
    GC_Actor::Kill(world);
}

///////////////////////////////////////////////////////////////////////////////

GC_Explosion& MakeExplosionStandard(World &world, const vec2d &pos, GC_Player *owner)
{
	float duration = 0.32f;
	
	auto main = new GC_ParticleExplosion(world, vec2d(0,0), PARTICLE_EXPLOSION1, duration, vrand(1));
	main->Register(world);
	main->MoveTo(world, pos);
	
	auto e = new GC_Explosion(world, owner, duration);
	e->Register(world);
	e->MoveTo(world, pos);
	e->SetRadius(70);
	e->SetDamage(150);
	e->SetBoomTimeout(0.03f);
	
	for(int n = 0; n < 28; ++n)
	{
		//ring
		float ang = frand(PI2);
		auto p = new GC_Particle(world, vec2d(ang) * 100, PARTICLE_TYPE1, frand(0.5f) + 0.1f);
        p->Register(world);
        p->MoveTo(world, pos);
		
		//smoke
		ang = frand(PI2);
		float d = frand(64.0f) - 32.0f;
		
		auto p1 = new GC_Particle(world, SPEED_SMOKE, PARTICLE_SMOKE, 1.5f);
        p1->Register(world);
        p1->MoveTo(world, pos + vec2d(ang) * d);
        p1->_time = frand(1.0f);
	}
	GC_Particle *p = new GC_ParticleDecal(world, vec2d(0,0), PARTICLE_SMALLBLAST, 8.0f, vrand(1));
    p->Register(world);
    p->MoveTo(world, pos);
	p->SetFade(true);
	
	PLAY(SND_BoomStandard, pos);
	
	return *e;
}

GC_Explosion& MakeExplosionBig(World &world, const vec2d &pos, GC_Player *owner)
{
	float duration = 0.72f;
	
	auto main = new GC_ParticleExplosion(world, vec2d(0,0), PARTICLE_EXPLOSION2, duration, vrand(1));
	main->Register(world);
	main->MoveTo(world, pos);

	auto e = new GC_Explosion(world, owner, duration);
	e->Register(world);
	e->MoveTo(world, pos);
	e->SetRadius(128);
	e->SetDamage(90);
	e->SetBoomTimeout(0.10f);
	
	for( int n = 0; n < 80; ++n )
	{
		//ring
		for( int i = 0; i < 2; ++i )
		{
			auto p = new GC_Particle(world, vrand((200.0f + frand(30.0f)) * 0.9f), PARTICLE_TYPE1, frand(0.6f) + 0.1f);
            p->Register(world);
            p->MoveTo(world, pos + vrand(frand(20.0f)));
		}

		vec2d a;

		//dust
		a = vrand(frand(40.0f));
		auto p = new GC_Particle(world, a * 2, PARTICLE_TYPE2, frand(0.5f) + 0.25f);
        p->Register(world);
        p->MoveTo(world, pos + a);

		// sparkles
		a = vrand(1);
		auto p1 = new GC_Particle(world, a * frand(80.0f), PARTICLE_TRACE1, frand(0.3f) + 0.2f, a);
        p1->Register(world);
        p1->MoveTo(world, pos + a * frand(40.0f));

		//smoke
		a = vrand(frand(48.0f));
		auto p2 = new GC_Particle(world, SPEED_SMOKE + a * 0.5f, PARTICLE_SMOKE, 1.5f);
        p2->Register(world);
        p2->MoveTo(world, pos + a);
        p2->_time = frand(1.0f);
	}

	GC_Particle *p = new GC_ParticleDecal(world, vec2d(0,0), PARTICLE_BIGBLAST, 20.0f, vrand(1));
    p->Register(world);
    p->MoveTo(world, pos);
	p->SetFade(true);

	PLAY(SND_BoomBig, pos);

	return *e;
}

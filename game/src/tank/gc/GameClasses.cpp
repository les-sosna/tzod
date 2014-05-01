//GameClasses.cpp

#include "GameClasses.h"

#include "Camera.h"
#include "Level.h"
#include "Macros.h"
#include "MapFile.h"
#include "particles.h"
#include "Player.h"
#include "SaveFile.h"
#include "Sound.h"
#include "Vehicle.h"

#include "core/Debug.h"
#include "video/RenderBase.h"
#include "config/Config.h"


/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wood)
{
	ED_LAND( "wood", "obj_wood",  7 );
	return true;
}

GC_Wood::GC_Wood(Level &world, float xPos, float yPos)
  : GC_2dSprite(world)
{
	AddContext( &world.grid_wood );

	SetZ(world, Z_WOOD);

	SetTexture("wood");
	MoveTo(world, vec2d(xPos, yPos));
	SetFrame(4);

	_tile = 0;
	UpdateTile(world, true);
}

GC_Wood::GC_Wood(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Wood::~GC_Wood()
{
}

void GC_Wood::Kill(Level &world)
{
    UpdateTile(world, false);
    GC_2dSprite::Kill(world);
}

void GC_Wood::UpdateTile(Level &world, bool flag)
{
	static char tile1[9] = {5, 6, 7, 4,-1, 0, 3, 2, 1};
	static char tile2[9] = {1, 2, 3, 0,-1, 4, 7, 6, 5};
	///////////////////////////////////////////////////
	FRECT frect;
	GetGlobalRect(frect);
	frect.left   = frect.left / LOCATION_SIZE - 0.5f;
	frect.top    = frect.top  / LOCATION_SIZE - 0.5f;
	frect.right  = frect.right  / LOCATION_SIZE + 0.5f;
	frect.bottom = frect.bottom / LOCATION_SIZE + 0.5f;

	PtrList<ObjectList> receive;
	world.grid_wood.OverlapRect(receive, frect);
	///////////////////////////////////////////////////
	PtrList<ObjectList>::iterator rit = receive.begin();
	for( ; rit != receive.end(); ++rit )
	{
		ObjectList::iterator it = (*rit)->begin();
		for( ; it != (*rit)->end(); ++it )
		{
			GC_Wood *object = static_cast<GC_Wood *>(*it);
			if( this == object ) continue;

			vec2d dx = (GetPos() - object->GetPos()) / CELL_SIZE;
			if( dx.sqr() < 2.5f )
			{
				int x = int(dx.x + 1.5f);
				int y = int(dx.y + 1.5f);

				object->SetTile(tile1[x + y * 3], flag);
				SetTile(tile2[x + y * 3], flag);
			}
		}
	}
}

void GC_Wood::Serialize(Level &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);

	f.Serialize(_tile);

	if( f.loading() )
		AddContext(&world.grid_wood);
}

void GC_Wood::Draw(bool editorMode) const
{
	static const float dx[8]   = { 32, 32,  0,-32,-32,-32,  0, 32 };
	static const float dy[8]   = {  0, 32, 32, 32,  0,-32,-32,-32 };
	static const int frames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };

	vec2d pos = GetPos();

	if( !editorMode )
	{
		for( int i = 0; i < 8; ++i )
		{
			if( 0 == (_tile & (1 << i)) )
			{
				g_texman->DrawSprite(GetTexture(), frames[i], 0xffffffff, pos.x + dx[i], pos.y + dy[i], GetDirection());
			}
		}
		g_texman->DrawSprite(GetTexture(), 4, 0xffffffff, pos.x, pos.y, GetDirection());
	}
	else
	{
		g_texman->DrawSprite(GetTexture(), 4, 0x7f7f7f7f, pos.x, pos.y, GetDirection());
	}
}

void GC_Wood::SetTile(char nTile, bool value)
{
	assert(0 <= nTile && nTile < 8);

	if( value )
		_tile |=  (1 << nTile);
	else
		_tile &= ~(1 << nTile);
}

///////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Explosion)
{
	return true;
}

GC_Explosion::GC_Explosion(Level &world, GC_Player *owner)
  : GC_2dSprite(world)
  , _boomOK(false)
  , _owner(owner)
  , _light(new GC_Light(world, GC_Light::LIGHT_POINT))
  , _time(0)
  , _time_life(0.5f)
  , _time_boom(0)
  , _damage(1)
  , _radius(32)
{
	SetZ(world, Z_EXPLODE);
	SetDirection(vrand(1));
	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Explosion::GC_Explosion(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Explosion::~GC_Explosion()
{
}

void GC_Explosion::Serialize(Level &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);
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

void GC_Explosion::Boom(Level &world, float radius, float damage)
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
	PtrList<ObjectList> receive;
	FRECT rt = {GetPos().x - radius, GetPos().y - radius, GetPos().x + radius, GetPos().y + radius};
	rt.left   /= LOCATION_SIZE;
	rt.top    /= LOCATION_SIZE;
	rt.right  /= LOCATION_SIZE;
	rt.bottom /= LOCATION_SIZE;
	world.grid_rigid_s.OverlapRect(receive, rt);

	//
	// prepare the field for tracing
	//
	PtrList<ObjectList>::iterator it = receive.begin();
	for( ; it != receive.end(); ++it )
	{
		ObjectList::iterator cdit = (*it)->begin();
		while( (*it)->end() != cdit )
		{
			GC_RigidBodyStatic *pDamObject = (GC_RigidBodyStatic *) (*cdit);
			++cdit;

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
	for( it = receive.begin(); it != receive.end(); ++it )
	{
		FOREACH_SAFE(**it, GC_RigidBodyStatic, pDamObject)
		{
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
						FIELD_TYPE::iterator it = field.begin();
						while( it != field.end() )
							(it++)->second.checked = false;
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
                    pDamObject->TakeDamage(world, dam, GetPos(), _owner);
				}
			}
		}
	}

	_owner = NULL;
	_boomOK = true;
}

void GC_Explosion::TimeStepFixed(Level &world, float dt)
{
	GC_2dSprite::TimeStepFixed(world, dt);

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
		SetVisible(world, false);
	}
	else
	{
		SetFrame(int((float)GetFrameCount() * _time / _time_life));
	}

	_light->SetIntensity(1.0f - powf(_time / (_time_life * 1.5f - _time_boom), 6));
}

void GC_Explosion::Kill(Level &world)
{
	SAFE_KILL(world, _light);
    GC_2dSprite::Kill(world);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Boom_Standard)
{
	return true;
}

GC_Boom_Standard::GC_Boom_Standard(Level &world, const vec2d &pos, GC_Player *owner)
  : GC_Explosion(world, owner)
{
	static const TextureCache tex1("particle_1");
	static const TextureCache tex2("particle_smoke");
	static const TextureCache tex3("smallblast");

	_radius = 70;
	_damage    = 150;

	_time_life = 0.32f;
	_time_boom = 0.03f;

	SetTexture("explosion_o");
	MoveTo( world, pos);

	for(int n = 0; n < 28; ++n)
	{
		//ring
		float ang = frand(PI2);
		new GC_Particle(world, pos, vec2d(ang) * 100, tex1, frand(0.5f) + 0.1f);

		//smoke
		ang = frand(PI2);
		float d = frand(64.0f) - 32.0f;

		(new GC_Particle(world, GetPos() + vec2d(ang) * d, SPEED_SMOKE, tex2, 1.5f))
			->_time = frand(1.0f);
	}
	GC_Particle *p = new GC_Particle(world, GetPos(), vec2d(0,0), tex3, 8.0f, vrand(1));
	p->SetZ(world, Z_WATER);
	p->SetFade(true);

	_light->SetRadius(_radius * 5);
	_light->MoveTo(world, GetPos());

	PLAY(SND_BoomStandard, GetPos());
}

GC_Boom_Standard::GC_Boom_Standard(FromFile)
  : GC_Explosion(FromFile())
{
}

GC_Boom_Standard::~GC_Boom_Standard()
{
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Boom_Big)
{
	return true;
}

GC_Boom_Big::GC_Boom_Big(Level &world, const vec2d &pos, GC_Player *owner)
  : GC_Explosion(world, owner)
{
	static const TextureCache tex1("particle_1");
	static const TextureCache tex2("particle_2");
	static const TextureCache tex4("particle_trace");
	static const TextureCache tex5("particle_smoke");
	static const TextureCache tex6("bigblast");

	_radius = 128;
	_damage    = 90;

	_time_life = 0.72f;
	_time_boom = 0.10f;

	SetTexture("explosion_big");
	MoveTo(world, pos);

	for( int n = 0; n < 80; ++n )
	{
		//ring
		for( int i = 0; i < 2; ++i )
		{
			new GC_Particle(world, GetPos() + vrand(frand(20.0f)),
				vrand((200.0f + frand(30.0f)) * 0.9f), tex1, frand(0.6f) + 0.1f);
		}

		vec2d a;

		//dust
		a = vrand(frand(40.0f));
		new GC_Particle(world, GetPos() + a, a * 2, tex2, frand(0.5f) + 0.25f);

		// sparkles
		a = vrand(1);
		new GC_Particle(world, GetPos() + a * frand(40.0f), a * frand(80.0f), tex4, frand(0.3f) + 0.2f, a);

		//smoke
		a = vrand(frand(48.0f));
		(new GC_Particle(world, GetPos() + a, SPEED_SMOKE + a * 0.5f, tex5, 1.5f))->_time = frand(1.0f);
	}

	GC_Particle *p = new GC_Particle(world, GetPos(), vec2d(0,0), tex6, 20.0f, vrand(1));
	p->SetZ(world, Z_WATER);
	p->SetFade(true);

	_light->SetRadius(_radius * 5);
	_light->MoveTo(world, GetPos());

	PLAY(SND_BoomBig, GetPos());
}

GC_Boom_Big::GC_Boom_Big(FromFile)
  : GC_Explosion(FromFile())
{
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HealthDaemon)
{
	return true;
}

GC_HealthDaemon::GC_HealthDaemon(Level &world,
                                 GC_RigidBodyStatic *victim,
                                 GC_Player *owner,
                                 float damage, float time)
  : GC_2dSprite(world)
  , _time(time)
  , _damage(damage)
  , _victim(victim)
  , _owner(owner)
{
	assert(victim);

	_victim->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_HealthDaemon::OnVictimMove);
	_victim->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_HealthDaemon::OnVictimKill);

	MoveTo(world, _victim->GetPos());
	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_HealthDaemon::GC_HealthDaemon(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_HealthDaemon::~GC_HealthDaemon()
{
}

void GC_HealthDaemon::Serialize(Level &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);

	f.Serialize(_time);
	f.Serialize(_damage);
	f.Serialize(_victim);
	f.Serialize(_owner);
}

void GC_HealthDaemon::TimeStepFloat(Level &world, float dt)
{
}

void GC_HealthDaemon::TimeStepFixed(Level &world, float dt)
{
	_time -= dt;
	bool bKill = false;
	if( _time <= 0 )
	{
		dt += _time;
		bKill = true;
	}
	if( !_victim->TakeDamage(world, dt * _damage, _victim->GetPos(), _owner) && bKill )
		Kill(world); // victim has died
}

void GC_HealthDaemon::OnVictimMove(Level &world, GC_Object *sender, void *param)
{
	MoveTo(world, static_cast<GC_Actor*>(sender)->GetPos());
}

void GC_HealthDaemon::OnVictimKill(Level &world, GC_Object *sender, void *param)
{
	Kill(world);
}

/////////////////////////////////////////////////////////////
//class GC_Text - text drawing

IMPLEMENT_SELF_REGISTRATION(GC_Text)
{
	return true;
}

GC_Text::GC_Text(Level &world, int xPos, int yPos, const std::string &text, enumAlignText align)
  : GC_2dSprite(world)
{
	SetFont("font_default");
	SetText(text);
	SetAlign(align);

	MoveTo(world, vec2d((float)xPos, (float)yPos));
}

void GC_Text::SetText(const std::string &text)
{
	_text = text;
}

void GC_Text::SetFont(const char *fontname)
{
	SetTexture(fontname);
}

void GC_Text::SetAlign(enumAlignText align)
{
	_align = align;
}

void GC_Text::Serialize(Level &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);
	f.Serialize(_text);
	f.Serialize(_align);
}

void GC_Text::Draw(bool editorMode) const
{
	vec2d pos = GetPos();
	g_texman->DrawBitmapText(pos.x, pos.y, GetTexture(), GetColor(), _text, _align);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Text_ToolTip)
{
	return true;
}

GC_Text_ToolTip::GC_Text_ToolTip(Level &world, vec2d pos, const std::string &text, const char *font)
  : GC_Text(world, int(pos.x), int(pos.y), text, alignTextCC)
{
	_time = 0;

	SetZ(world, Z_PARTICLE);

	SetText(text);
	SetFont(font);

	float x_min = (float) (GetSpriteWidth() / 2);
	float x_max = world._sx - x_min;

	_y0 = pos.y;
	MoveTo(world, vec2d(std::min(x_max, std::max(x_min, GetPos().x)) - (GetSpriteWidth() / 2), GetPos().y));

	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

void GC_Text_ToolTip::Serialize(Level &world, SaveFile &f)
{
	GC_Text::Serialize(world, f);
	f.Serialize(_time);
	f.Serialize(_y0);
}

void GC_Text_ToolTip::TimeStepFloat(Level &world, float dt)
{
	_time += dt;
	MoveTo(world, vec2d(GetPos().x, _y0 - _time * 20.0f));
	if( _time > 1.2f )
    {
        Kill(world);
    }
}

///////////////////////////////////////////////////////////////////////////////
// end of file

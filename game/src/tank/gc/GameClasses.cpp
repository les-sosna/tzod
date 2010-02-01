//GameClasses.cpp

#include "stdafx.h"

#include "core/Debug.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "ui/GuiManager.h"

#include "video/RenderBase.h"

#include "config/Config.h"

#include "functions.h"
#include "Macros.h"
#include "Level.h"

#include "Player.h"
#include "GameClasses.h"
#include "Camera.h"
#include "Vehicle.h"
#include "Sound.h"
#include "particles.h"

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wood)
{
	ED_LAND( "wood", "obj_wood",  7 );
	return true;
}

GC_Wood::GC_Wood(float xPos, float yPos)
  : GC_2dSprite()
{
	AddContext( &g_level->grid_wood );

	SetZ(Z_WOOD);

	SetTexture("wood");
	MoveTo( vec2d(xPos, yPos) );
	SetFrame(4);

	_tile = 0;
	UpdateTile(true);
}

GC_Wood::GC_Wood(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_Wood::UpdateTile(bool flag)
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
	g_level->grid_wood.OverlapRect(receive, frect);
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

void GC_Wood::Kill()
{
	UpdateTile(false);
	GC_2dSprite::Kill();
}

void GC_Wood::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_tile);

	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_wood);
}

void GC_Wood::Draw() const
{
	static const float dx[8]   = { 32, 32,  0,-32,-32,-32,  0, 32 };
	static const float dy[8]   = {  0, 32, 32, 32,  0,-32,-32,-32 };
	static const int frames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };

	vec2d pos = GetPosPredicted();

	if( !g_level->GetEditorMode() )
	{
		for( char i = 0; i < 8; ++i )
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

GC_Explosion::GC_Explosion(GC_Player *owner)
  : GC_2dSprite()
  , _owner(owner)
  , _light(new GC_Light(GC_Light::LIGHT_POINT))
  , _time(0)
  , _time_life(0.5f)
  , _time_boom(0)
  , _damage(1)
  , _radius(32)
  , _boomOK(false)
{
	SetZ(Z_EXPLODE);
	SetDirection(vrand(1));
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Explosion::GC_Explosion(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Explosion::~GC_Explosion()
{
	assert(!_owner);
}

void GC_Explosion::Kill()
{
	_owner = NULL;
	SAFE_KILL(_light);
	GC_2dSprite::Kill();
}

void GC_Explosion::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
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

void GC_Explosion::Boom(float radius, float damage)
{
	FOREACH( g_level->GetList(LIST_cameras), GC_Camera, pCamera )
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
	g_level->grid_rigid_s.OverlapRect(receive, rt);

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

			if( pDamObject->IsKilled() ) continue;
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
			if( pDamObject->CheckFlags(GC_FLAG_RBSTATIC_PHANTOM|GC_FLAG_OBJECT_KILLED) )
			{
				continue;
			}

			vec2d dir = pDamObject->GetPos() - GetPos();
			float d = dir.len();

			if( d <= radius)
			{
				GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) g_level->TraceNearest(
					g_level->grid_rigid_s, NULL, GetPos(), dir);

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
					float dam = __max(0, damage * (1 - d / radius));
					assert(dam >= 0);
					if( GC_RigidBodyDynamic *dyn = dynamic_cast<GC_RigidBodyDynamic *>(pDamObject) )
					{
						if( d > 1e-5 )
						{
							dyn->ApplyImpulse(dir * (dam / d), dyn->GetPos());
						}
					}
					if( !pDamObject->CheckFlags(GC_FLAG_RBSTATIC_PHANTOM) )
					{
						pDamObject->TakeDamage( dam, GetPos(), GetRawPtr(_owner) );
					}
				}
			}
		}
	}

	_owner = NULL;
	_boomOK = true;
}

void GC_Explosion::TimeStepFixed(float dt)
{
	GC_2dSprite::TimeStepFixed(dt);

	_time += dt;
	if( _time >= _time_boom && !_boomOK )
		Boom(_radius, _damage);

	if( _time >= _time_life )
	{
		if( _time >= _time_life * 1.5f )
		{
			Kill();
			return;
		}
		SetVisible(false);
	}
	else
	{
		SetFrame(int((float)GetFrameCount() * _time / _time_life));
	}

	_light->SetIntensity(1.0f - powf(_time / (_time_life * 1.5f - _time_boom), 6));
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Boom_Standard)
{
	return true;
}

GC_Boom_Standard::GC_Boom_Standard(const vec2d &pos, GC_Player *owner)
  : GC_Explosion(owner)
{
	static const TextureCache tex1("particle_1");
	static const TextureCache tex2("particle_smoke");
	static const TextureCache tex3("smallblast");

	_radius = 70;
	_damage    = 150;

	_time_life = 0.32f;
	_time_boom = 0.03f;

	SetTexture("explosion_o");
	MoveTo( pos );

	if( g_conf.g_particles.Get() )
	{
		for(int n = 0; n < 28; ++n)
		{
			//ring
			float ang = frand(PI2);
			new GC_Particle(pos, vec2d(ang) * 100, tex1, frand(0.5f) + 0.1f);

			//smoke
			ang = frand(PI2);
			float d = frand(64.0f) - 32.0f;

			(new GC_Particle(GetPos() + vec2d(ang) * d, SPEED_SMOKE, tex2, 1.5f))
				->_time = frand(1.0f);
		}
		GC_Particle *p = new GC_Particle(GetPos(), vec2d(0,0), tex3, 8.0f, vrand(1));
		p->SetZ(Z_WATER);
		p->SetFade(true);
	}

	_light->SetRadius(_radius * 5);
	_light->MoveTo(GetPos());

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

GC_Boom_Big::GC_Boom_Big(const vec2d &pos, GC_Player *owner)
  : GC_Explosion(owner)
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
	MoveTo( pos );

	if( g_conf.g_particles.Get() )
	{
		for( int n = 0; n < 80; ++n )
		{
			//ring
			for( int i = 0; i < 2; ++i )
			{
				new GC_Particle(GetPos() + vrand(frand(20.0f)),
					vrand((200.0f + frand(30.0f)) * 0.9f), tex1, frand(0.6f) + 0.1f);
			}

			vec2d a;

			//dust
			a = vrand(frand(40.0f));
			new GC_Particle(GetPos() + a, a * 2, tex2, frand(0.5f) + 0.25f);

			// sparkles
			a = vrand(1);
			new GC_Particle(GetPos() + a * frand(40.0f), a * frand(80.0f), tex4, frand(0.3f) + 0.2f, a);

			//smoke
			a = vrand(frand(48.0f));
			(new GC_Particle(GetPos() + a, SPEED_SMOKE + a * 0.5f, tex5, 1.5f))->_time = frand(1.0f);
		}

		GC_Particle *p = new GC_Particle(GetPos(), vec2d(0,0), tex6, 20.0f, vrand(1));
		p->SetZ(Z_WATER);
		p->SetFade(true);
	}

	_light->SetRadius(_radius * 5);
	_light->MoveTo(GetPos());

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

GC_HealthDaemon::GC_HealthDaemon(GC_RigidBodyStatic *victim, 
                                 GC_Player *owner,
                                 float damage, float time)
  : _time(time)
  , _damage(damage)
  , _victim(victim)
  , _owner(owner)
{
	assert(victim);
	assert(!victim->IsKilled());

	_victim->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_HealthDaemon::OnVictimMove);
	_victim->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_HealthDaemon::OnVictimKill);

	MoveTo(_victim->GetPos());
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED /*| GC_FLAG_OBJECT_EVENTS_TS_FLOATING*/ );
}

GC_HealthDaemon::GC_HealthDaemon(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_HealthDaemon::~GC_HealthDaemon()
{
}

void GC_HealthDaemon::Kill()
{
	_victim = NULL;
	_owner  = NULL;
	GC_2dSprite::Kill();
}

void GC_HealthDaemon::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_time);
	f.Serialize(_damage);
	f.Serialize(_victim);
	f.Serialize(_owner);
}

void GC_HealthDaemon::TimeStepFloat(float dt)
{
}

void GC_HealthDaemon::TimeStepFixed(float dt)
{
	_time -= dt;
	bool bKill = false;
	if( _time <= 0 )
	{
		dt += _time;
		bKill = true;
	}
	if( !_victim->TakeDamage(dt * _damage, _victim->GetPos(), GetRawPtr(_owner)) && bKill )
		Kill(); // victim has died
}

void GC_HealthDaemon::OnVictimMove(GC_Object *sender, void *param)
{
	MoveTo(static_cast<GC_Actor*>(sender)->GetPos());
}

void GC_HealthDaemon::OnVictimKill(GC_Object *sender, void *param)
{
	Kill();
}

/////////////////////////////////////////////////////////////
//class GC_Text - text drawing

IMPLEMENT_SELF_REGISTRATION(GC_Text)
{
	return true;
}

GC_Text::GC_Text(int xPos, int yPos, const string_t &text, enumAlignText align)
  : GC_2dSprite()
{
	SetFont("font_default");
	SetText(text);
	SetAlign(align);

	MoveTo( vec2d((float)xPos, (float)yPos) );
}

void GC_Text::SetText(const string_t &text)
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

void GC_Text::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_text);
	f.Serialize(_align);
}

void GC_Text::Draw() const
{
	vec2d pos = GetPosPredicted();
	g_texman->DrawBitmapText(pos.x, pos.y, GetTexture(), GetColor(), _text, _align);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Text_ToolTip)
{
	return true;
}

GC_Text_ToolTip::GC_Text_ToolTip(vec2d pos, const string_t &text, const char *font)
  : GC_Text(int(pos.x), int(pos.y), text, alignTextCC)
{
	_time = 0;

	SetZ(Z_PARTICLE);

	SetText(text);
	SetFont(font);

	float x_min = (float) (GetSpriteWidth() / 2);
	float x_max = g_level->_sx - x_min;

	_y0 = pos.y;
	MoveTo( vec2d(__min(x_max, __max(x_min, GetPos().x)) - (GetSpriteWidth() / 2), GetPos().y) );

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
}

void GC_Text_ToolTip::Serialize(SaveFile &f)
{
	GC_Text::Serialize(f);
	f.Serialize(_time);
	f.Serialize(_y0);
}

void GC_Text_ToolTip::TimeStepFloat(float dt)
{
	_time += dt;
	MoveTo(vec2d(GetPos().x, _y0 - _time * 20.0f));
	if( _time > 1.2f ) Kill();
}

///////////////////////////////////////////////////////////////////////////////
// end of file

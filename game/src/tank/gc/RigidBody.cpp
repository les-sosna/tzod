// RigidBody.cpp

#include "stdafx.h"

#include "RigidBody.h"

#include "level.h"
#include "functions.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"
#include "config/Config.h"
#include "core/Console.h"

#include "Sound.h"
#include "Particles.h"

///////////////////////////////////////////////////////////////////////////////

GC_RigidBodyStatic::GC_RigidBodyStatic()
  : GC_2dSprite()
  , _health(1)
  , _health_max(1)
  , _direction(1, 0)
{
	AddContext(&g_level->grid_rigid_s);
	ZeroMemory(&_vertices, sizeof(_vertices));
}

GC_RigidBodyStatic::GC_RigidBodyStatic(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_RigidBodyStatic::SetHealth(float cur, float max)
{
	_ASSERT(cur <= max);
	_health = cur;
	_health_max = max;
	PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
}

void GC_RigidBodyStatic::SetHealthCur(float hp)
{
	_ASSERT(hp <= _health_max);
	_health = hp;
	PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
}

void GC_RigidBodyStatic::SetHealthMax(float hp)
{
	_ASSERT(hp >= _health);
	_health_max = hp;
	PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
}

void GC_RigidBodyStatic::OnDestroy()
{
	PulseNotify(NOTIFY_RIGIDBODY_DESTROY);
	if( !_scriptOnDestroy.empty() )
	{
		script_exec(g_env.L, _scriptOnDestroy.c_str());
	}
}

bool GC_RigidBodyStatic::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	_ASSERT(!IsKilled());

	if( CheckFlags(GC_FLAG_RBSTATIC_DESTROYED) )
	{
		return true;
	}

	if( _health_max > 0 )
	{
		SetHealthCur(GetHealth() - damage);

		if( !_scriptOnDamage.empty() )
		{
			AddRef();
			script_exec(g_env.L, _scriptOnDamage.c_str());
			if( IsKilled() )
			{
				Release();
				return true;
			}
			Release();
		}

		if( GetHealth() <= 0 )
		{
			AddRef();
			SetFlags(GC_FLAG_RBSTATIC_DESTROYED);
			OnDestroy();
			Kill();
			Release();
			return true;
		}
	}
	return false;
}

void GC_RigidBodyStatic::AlignToTexture()
{
	_vertices[0].x =  GetSpriteWidth()  * 0.5f;
	_vertices[0].y = -GetSpriteHeight() * 0.5f;
	_vertices[1].x =  GetSpriteWidth()  * 0.5f;
	_vertices[1].y =  GetSpriteHeight() * 0.5f;
	_vertices[2].x = -GetSpriteWidth()  * 0.5f;
	_vertices[2].y =  GetSpriteHeight() * 0.5f;
	_vertices[3].x = -GetSpriteWidth()  * 0.5f;
	_vertices[3].y = -GetSpriteHeight() * 0.5f;

	_radius = sqrtf( GetSpriteWidth() * GetSpriteWidth()
		+ GetSpriteHeight() * GetSpriteHeight() ) * 0.5f;
}

void GC_RigidBodyStatic::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);

	MAP_EXCHANGE_FLOAT(  health,     _health,     GetDefaultHealth());
	MAP_EXCHANGE_FLOAT(  health_max, _health_max, GetDefaultHealth());
	MAP_EXCHANGE_STRING( on_destroy, _scriptOnDestroy, "");
	MAP_EXCHANGE_STRING( on_damage,  _scriptOnDamage,  "");

	if( f.loading() )
	{
		_health = __min(_health, _health_max);
		PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
	}
}

void GC_RigidBodyStatic::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_scriptOnDestroy);
	f.Serialize(_scriptOnDamage);
	f.Serialize(_health);
	f.Serialize(_health_max);
	f.Serialize(_radius);
	f.Serialize(_direction);
	f.SerializeArray(_vertices, 4);

	if( !IsKilled() && f.loading() && GetPassability() > 0 )
		g_level->_field.ProcessObject(this, true);

	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_rigid_s);
}

void GC_RigidBodyStatic::Kill()
{
	if( GetPassability() > 0 )
		g_level->_field.ProcessObject(this, false);
	GC_2dSprite::Kill();
}


PropertySet* GC_RigidBodyStatic::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_RigidBodyStatic::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propOnDestroy( ObjectProperty::TYPE_STRING,   "on_destroy"  )
  , _propOnDamage(  ObjectProperty::TYPE_STRING,   "on_damage"   )
  , _propHealth(    ObjectProperty::TYPE_FLOAT,    "health"      )
  , _propMaxHealth( ObjectProperty::TYPE_FLOAT,    "max_health"  )
{
	_propMaxHealth.SetFloatRange(0, 100000);
	_propHealth.SetFloatRange(0, 100000);
}

int GC_RigidBodyStatic::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 4;
}

ObjectProperty* GC_RigidBodyStatic::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propOnDestroy;
	case 1: return &_propOnDamage;
	case 2: return &_propHealth;
	case 3: return &_propMaxHealth;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_RigidBodyStatic::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_RigidBodyStatic *tmp = static_cast<GC_RigidBodyStatic *>(GetObject());

	if( applyToObject )
	{
		tmp->_scriptOnDestroy = _propOnDestroy.GetStringValue();
		tmp->_scriptOnDamage  = _propOnDamage.GetStringValue();
		tmp->SetHealth( __min(_propMaxHealth.GetFloatValue(), _propHealth.GetFloatValue()),
		                _propMaxHealth.GetFloatValue() );
	}
	else
	{
		_propHealth.SetFloatValue(tmp->GetHealth());
		_propMaxHealth.SetFloatValue(tmp->GetHealthMax());
		_propOnDestroy.SetStringValue(tmp->_scriptOnDestroy);
		_propOnDamage.SetStringValue(tmp->_scriptOnDamage);
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wall)
{
	ED_LAND("wall_brick", "obj_wall_brick",  2 );
	return true;
}

GC_Wall::GC_Wall(float xPos, float yPos)
  : GC_RigidBodyStatic()
{
	AddContext(&g_level->grid_walls);
	SetZ(Z_WALLS);
	SetHealth(50, 50);

	SetTexture("brick_wall");

	Resize(CELL_SIZE, CELL_SIZE);
	CenterPivot();
	AlignToTexture();
	MoveTo( vec2d(xPos, yPos) );

	g_level->_field.ProcessObject(this, true);
}

GC_Wall::GC_Wall(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_Wall::~GC_Wall()
{
}

void GC_Wall::Kill()
{
	SetCorner(0);
	GC_RigidBodyStatic::Kill();
}

void GC_Wall::mapExchange(MapFile &f)
{
	GC_RigidBodyStatic::mapExchange(f);
	int corner = GetCorner();
	MAP_EXCHANGE_INT(corner, corner, 0);

	if( f.loading() )
	{
		SetCorner(corner % 5);
	}
}

void GC_Wall::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);

	if( !IsKilled() && f.loading() )
	{
		AddContext(&g_level->grid_walls);
		if( CheckFlags(GC_FLAG_WALL_CORNER_ALL) )
		{
			vec2d p = GetPos() / CELL_SIZE;
			int x;
			int y;
			if( CheckFlags(GC_FLAG_WALL_CORNER_LT) )
			{
				x = int(p.x+1);
				y = int(p.y+1);
			}
			if( CheckFlags(GC_FLAG_WALL_CORNER_RT) )
			{
				x = int(p.x);
				y = int(p.y + 1);
			}
			if( CheckFlags(GC_FLAG_WALL_CORNER_LB) )
			{
				x = int(p.x + 1);
				y = int(p.y);
			}
			if( CheckFlags(GC_FLAG_WALL_CORNER_RB) )
			{
				x = int(p.x);
				y = int(p.y);
			}
			g_level->_field(x, y).RemoveObject(this);
			if( 0 == x || 0 == y || g_level->_field.GetX() - 1 == x || g_level->_field.GetX() - 1 == y )
			{
				g_level->_field(x, y)._prop = 0xFF;
			}
		}
	}
}

void GC_Wall::OnDestroy()
{
	static const TextureCache tex("particle_smoke");

	PLAY(SND_WallDestroy, GetPos());

	if( g_conf->g_particles->Get() )
	{
		for( int n = 0; n < 5; ++n )
		{
			(new GC_Brick_Fragment_01( GetPos() + vrand(GetRadius()),
				vec2d(frand(100.0f) - 50, -frand(100.0f))
			))->SetShadow(true);
		}

		new GC_Particle(GetPos(), SPEED_SMOKE, tex, frand(0.2f) + 0.3f);
	}

	GC_RigidBodyStatic::OnDestroy();
}

bool GC_Wall::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	if( !GC_RigidBodyStatic::TakeDamage(damage, hit, from) && GetHealthMax() > 0 )
	{
		SetFrame((GetFrameCount()-1)-int((float)(GetFrameCount()-1)*GetHealth()/GetHealthMax()));
		if( g_conf->g_particles->Get() && damage >= DAMAGE_BULLET )
		{
			vec2d v = hit - GetPos();
			if( fabsf(v.x) > fabsf(v.y) )
			{
				v.x = v.x > 0 ? 50.0f : -50.0f;
				v.y = 0;
			}
			else
			{
				v.x = 0;
				v.y = v.y > 0 ? 50.0f : -50.0f;
			}
			v += vrand(25);

			(new GC_Brick_Fragment_01(hit, v))->SetShadow(true);
		}
		return false;
	}
	return true;
}

void GC_Wall::SetCorner(int index) // 0 means normal view
{
	_ASSERT(index >= 0 && index < 5);
	static const DWORD flags[] = {
		0,
		GC_FLAG_WALL_CORNER_LT,
		GC_FLAG_WALL_CORNER_RT,
		GC_FLAG_WALL_CORNER_RB,
		GC_FLAG_WALL_CORNER_LB
	};

	vec2d p = GetPos() / CELL_SIZE;
	int x;
	int y;
	if( CheckFlags(GC_FLAG_WALL_CORNER_ALL) )
	{
		if( CheckFlags(GC_FLAG_WALL_CORNER_LT) )
		{
			x = int(p.x+1);
			y = int(p.y+1);
		}
		if( CheckFlags(GC_FLAG_WALL_CORNER_RT) )
		{
			x = int(p.x);
			y = int(p.y + 1);
		}
		if( CheckFlags(GC_FLAG_WALL_CORNER_LB) )
		{
			x = int(p.x + 1);
			y = int(p.y);
		}
		if( CheckFlags(GC_FLAG_WALL_CORNER_RB) )
		{
			x = int(p.x);
			y = int(p.y);
		}
		g_level->_field(x, y).AddObject(this);
	}

	ClearFlags(GC_FLAG_WALL_CORNER_ALL);
	SetFlags(flags[index]);

	SetTexture(GetCornerTexture(index));
	Resize(CELL_SIZE, CELL_SIZE);
	CenterPivot();
	AlignToTexture();
	if( 0 != index ) _vertices[index&3].Set(0,0);

	if( CheckFlags(GC_FLAG_WALL_CORNER_ALL) )
	{
		if( CheckFlags(GC_FLAG_WALL_CORNER_LT) )
		{
			x = int(p.x+1);
			y = int(p.y+1);
		}
		if( CheckFlags(GC_FLAG_WALL_CORNER_RT) )
		{
			x = int(p.x);
			y = int(p.y + 1);
		}
		if( CheckFlags(GC_FLAG_WALL_CORNER_LB) )
		{
			x = int(p.x + 1);
			y = int(p.y);
		}
		if( CheckFlags(GC_FLAG_WALL_CORNER_RB) )
		{
			x = int(p.x);
			y = int(p.y);
		}
		g_level->_field(x, y).RemoveObject(this);
		if( 0 == x || 0 == y || g_level->_field.GetX() - 1 == x || g_level->_field.GetX() - 1 == y )
		{
			g_level->_field(x, y)._prop = 0xFF;
		}
	}
}

int GC_Wall::GetCorner(void)
{
	int index = 0;
	switch( GetFlags() & GC_FLAG_WALL_CORNER_ALL )
	{
	case 0:
		index = 0;
		break;
	case GC_FLAG_WALL_CORNER_LT:
		index = 1;
		break;
	case GC_FLAG_WALL_CORNER_RT:
		index = 2;
		break;
	case GC_FLAG_WALL_CORNER_RB:
		index = 3;
		break;
	case GC_FLAG_WALL_CORNER_LB:
		index = 4;
        break;
	default:
		_ASSERT(0);
	}
	return index;
}

const char* GC_Wall::GetCornerTexture(int i)
{
	_ASSERT(i >=0 && i < 5);
	static const char* tex[] = {
		"brick_wall",
		"brick_lt",
		"brick_rt",
		"brick_rb",
		"brick_lb"
	};
	return tex[i];
}

void GC_Wall::EditorAction()
{
	GC_2dSprite::EditorAction();
	SetCorner((GetCorner() + 1) % 5);
}


PropertySet* GC_Wall::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Wall::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propCorner( ObjectProperty::TYPE_INTEGER,   "corner"  )
{
	_propCorner.SetIntRange(0, 4);
}

int GC_Wall::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_Wall::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propCorner;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_Wall::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Wall *tmp = static_cast<GC_Wall *>(GetObject());

	if( applyToObject )
	{
		tmp->SetCorner(_propCorner.GetIntValue());
	}
	else
	{
		_propCorner.SetIntValue(tmp->GetCorner());
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wall_Concrete)
{
	ED_LAND("wall_concrete", "obj_wall_concrete", 1 );
	return true;
}

GC_Wall_Concrete::GC_Wall_Concrete(float xPos, float yPos)
  : GC_Wall(xPos, yPos)
{
	g_level->_field.ProcessObject(this, false);

	SetTexture("concrete_wall");
	Resize(CELL_SIZE, CELL_SIZE);
	CenterPivot();
	AlignToTexture();

	SetFrame(rand() % GetFrameCount());

	g_level->_field.ProcessObject(this, true);
}

bool GC_Wall_Concrete::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	if( damage >= DAMAGE_BULLET )
	{
		if( rand() < 256 )
			PLAY(SND_Hit1, hit);
		else if( rand() < 256 )
			PLAY(SND_Hit3, hit);
		else if( rand() < 256 )
			PLAY(SND_Hit5, hit);
	}
	return false;
}

const char* GC_Wall_Concrete::GetCornerTexture(int i)
{
	_ASSERT(i >=0 && i < 5);
	static const char* tex[] = {
		"concrete_wall",
		"concrete_lt",
		"concrete_rt",
		"concrete_rb",
		"concrete_lb"
	};
	return tex[i];
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Water)
{
	ED_LAND( "water", "obj_water", 0 );
	return true;
}

GC_Water::GC_Water(float xPos, float yPos)
  : GC_RigidBodyStatic()
{
	AddContext( &g_level->grid_water );

	SetZ(Z_WATER);

	SetTexture("water");
	Resize(CELL_SIZE, CELL_SIZE);
	CenterPivot();
	AlignToTexture();

	MoveTo( vec2d(xPos, yPos) );
	SetFrame(4);

	_tile = 0;
	UpdateTile(true);

	SetFlags(GC_FLAG_RBSTATIC_TRACE0);

	g_level->_field.ProcessObject(this, true);
}

GC_Water::GC_Water(FromFile)
  : GC_RigidBodyStatic(FromFile())
{
}

GC_Water::~GC_Water()
{
}

void GC_Water::UpdateTile(bool flag)
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

	PtrList<OBJECT_LIST> receive;
	g_level->grid_water.OverlapRect(receive, frect);
	///////////////////////////////////////////////////
	PtrList<OBJECT_LIST>::iterator rit = receive.begin();
	for( ; rit != receive.end(); ++rit )
	{
		OBJECT_LIST::iterator it = (*rit)->begin();
		for( ; it != (*rit)->end(); ++it )
		{
			GC_Water *object = (GC_Water *) (*it);
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

void GC_Water::Kill()
{
    UpdateTile(false);
	GC_RigidBodyStatic::Kill();
}

void GC_Water::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);

	f.Serialize(_tile);

	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_water);
}

void GC_Water::Draw()
{
	static float dx[8]   = {-32,-32,  0, 32, 32, 32,  0,-32 };
	static float dy[8]   = {  0,-32,-32,-32,  0, 32, 32, 32 };
	static int frames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };

	for( char i = 0; i < 8; ++i )
	{
		if( 0 == (_tile & (1 << i)) )
		{
			SetPivot(dx[i] + GetSpriteWidth() * 0.5f, dy[i] + GetSpriteHeight() * 0.5f);
			SetFrame(frames[i]);
			GC_RigidBodyStatic::Draw();
		}
	}

	CenterPivot();
	SetFrame(4);

	GC_RigidBodyStatic::Draw();
}

void GC_Water::SetTile(char nTile, bool value)
{
	_ASSERT(0 <= nTile && nTile < 8);

	if( value )
		_tile |= (1 << nTile);
	else
		_tile &= ~(1 << nTile);
}

bool GC_Water::TakeDamage(float damage, const vec2d &hit, GC_RigidBodyStatic *from)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

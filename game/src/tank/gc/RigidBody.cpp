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
#include "Projectiles.h"

///////////////////////////////////////////////////////////////////////////////

GC_RigidBodyStatic::GC_RigidBodyStatic() : GC_2dSprite()
{
	AddContext(&g_level->grid_rigid_s);

	_health     = 1;
	_health_max = 1;

	_direction.Set(1, 0);
}

GC_RigidBodyStatic::GC_RigidBodyStatic(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_RigidBodyStatic::SetHealth(float cur, float max)
{
	_health = cur;
	_health_max = max;
	PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
}

void GC_RigidBodyStatic::SetHealthCur(float hp)
{
	_health = hp;
	PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
}

void GC_RigidBodyStatic::SetHealthMax(float hp)
{
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

	if( f.loading() )
	{
		PulseNotify(NOTIFY_OBJECT_UPDATE_INDICATOR);
	}
}

void GC_RigidBodyStatic::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_scriptOnDestroy);
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

void GC_RigidBodyStatic::MoveTo(const vec2d &pos)
{
/*	if( GetPassability() > 0 )
	{
		g_level->_field.ProcessObject(this, false);
		GC_2dSprite::MoveTo(pos);
		g_level->_field.ProcessObject(this, true);
	}
	else
*/	{
		GC_2dSprite::MoveTo(pos);
	}
}

void GC_RigidBodyStatic::Kill()
{
	if( GetPassability() > 0 )
		g_level->_field.ProcessObject(this, false);
	GC_2dSprite::Kill();
}

SafePtr<PropertySet> GC_RigidBodyStatic::GetProperties()
{
	return new MyPropertySet(this);
}


GC_RigidBodyStatic::MyPropertySet::MyPropertySet(GC_Object *object)
: BASE(object)
, _propOnDestroy( ObjectProperty::TYPE_STRING,   "on_destroy"  )
, _propHealth(    ObjectProperty::TYPE_INTEGER,  "health"      )
, _propMaxHealth( ObjectProperty::TYPE_INTEGER,  "max_health"  )
{
	_propMaxHealth.SetRange(0, 10000);
	_propHealth.SetRange(0, 10000);

	Exchange(false);
}

int GC_RigidBodyStatic::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 3;
}

ObjectProperty* GC_RigidBodyStatic::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propOnDestroy;
	case 1: return &_propHealth;
	case 2: return &_propMaxHealth;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_RigidBodyStatic::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

	GC_RigidBodyStatic *tmp = static_cast<GC_RigidBodyStatic *>(GetObject());

	if( applyToObject )
	{
		tmp->_scriptOnDestroy = _propOnDestroy.GetValue();
		tmp->SetHealth((float) _propHealth.GetValueInt(), (float) _propMaxHealth.GetValueInt());
	}
	else
	{
		_propHealth.SetValueInt(int(tmp->GetHealth() + 0.5f));
		_propMaxHealth.SetValueInt(int(tmp->GetHealthMax() + 0.5f));
		_propOnDestroy.SetValue(tmp->_scriptOnDestroy);
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wall)
{
	ED_LAND("wall_brick", "Стена:\tКирпич",  0 );
	return true;
}

GC_Wall::GC_Wall(float xPos, float yPos) : GC_RigidBodyStatic()
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

GC_Wall::GC_Wall(FromFile) : GC_RigidBodyStatic(FromFile())
{
}

void GC_Wall::mapExchange(MapFile &f)
{
	GC_RigidBodyStatic::mapExchange(f);
	int corner = GetCornerView();
	MAP_EXCHANGE_INT(corner, corner, 0);

	if( f.loading() )
	{
		SetCornerView(corner % 5);
	}
}

void GC_Wall::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);

	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_walls);
}

void GC_Wall::OnDestroy()
{
	static const TextureCache tex("particle_smoke");

	PLAY(SND_WallDestroy, GetPos());

	if( g_conf.g_particles->Get() )
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
	if( !GC_RigidBodyStatic::TakeDamage(damage, hit, from) )
	{
		SetFrame((GetFrameCount()-1)-int((float)(GetFrameCount()-1)*GetHealth()/GetHealthMax()));
		if( g_conf.g_particles->Get() && damage >= DAMAGE_BULLET )
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

void GC_Wall::SetCornerView(int index) // 0 means normal view
{
	_ASSERT(index >= 0 && index < 5);
	static const DWORD flags[] = {
		0,
		GC_FLAG_WALL_CORNER_LT,
		GC_FLAG_WALL_CORNER_RT,
		GC_FLAG_WALL_CORNER_RB,
		GC_FLAG_WALL_CORNER_LB
	};

	ClearFlags(GC_FLAG_WALL_CORNER_ALL);
	SetFlags(flags[index]);

	SetTexture(getCornerTexture(index));
	Resize(CELL_SIZE, CELL_SIZE);
	CenterPivot();
	AlignToTexture();
	if( 0 != index ) _vertices[index&3].Set(0,0);
}

int GC_Wall::GetCornerView(void)
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

const char* GC_Wall::getCornerTexture(int i)
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
	SetCornerView((GetCornerView() + 1) % 5);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Wall_Concrete)
{
	ED_LAND("wall_concrete", "Стена:\tБетон", 0 );
	return true;
}

GC_Wall_Concrete::GC_Wall_Concrete(float xPos, float yPos) : GC_Wall(xPos, yPos)
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

const char* GC_Wall_Concrete::getCornerTexture(int i)
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
	ED_LAND( "water", "Ландшафт:\tВода", 0 );
	return true;
}

GC_Water::GC_Water(float xPos, float yPos) : GC_RigidBodyStatic()
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

GC_Water::GC_Water(FromFile) : GC_RigidBodyStatic(FromFile())
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
			if( dx.Square() < 2.5f )
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

std::vector<GC_RigidBodyDynamic::Contact> GC_RigidBodyDynamic::_contacts;
bool GC_RigidBodyDynamic::_glob_parity = false;


GC_RigidBodyDynamic::GC_RigidBodyDynamic() : GC_RigidBodyStatic()
{
	_lv.Zero();
	_av     = 0;
	_inv_m  = 1.0f / 1;
	_inv_i  = _inv_m * 12.0f / (36*36 + 36*36);
	_angle  = 0;

	_Nx     = 0;
	_Ny     = 0;
	_Nw     = 0;

	_Mx     = 0;
	_My     = 0;
	_Mw     = 0;

	_percussion = 1;
	_fragility  = 1;

	_external_force.Zero();
	_external_momentum = 0;

	_external_impulse.Zero();
	_external_torque = 0;


	if( _glob_parity ) SetFlags(GC_FLAG_RBDYMAMIC_PARITY);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_RigidBodyDynamic::GC_RigidBodyDynamic(FromFile) : GC_RigidBodyStatic(FromFile())
{
}

void GC_RigidBodyDynamic::Serialize(SaveFile &f)
{
	GC_RigidBodyStatic::Serialize(f);

	f.Serialize(_av);
	f.Serialize(_lv);
	f.Serialize(_inv_m);
	f.Serialize(_inv_i);
	f.Serialize(_angle);
	f.Serialize(_Nx);
	f.Serialize(_Ny);
	f.Serialize(_Nw);
	f.Serialize(_Mx);
	f.Serialize(_My);
	f.Serialize(_Mw);
	f.Serialize(_percussion);
	f.Serialize(_external_force);
	f.Serialize(_external_momentum);
	f.Serialize(_external_impulse);
	f.Serialize(_external_torque);
}

bool GC_RigidBodyDynamic::Intersect(GC_RigidBodyStatic *pObj, vec2d &origin, vec2d &normal)
{
	if( (pObj->GetPos() - GetPos()).Length() > GetRadius() + pObj->GetRadius() )
	{
		return false;
	}


	//
	// exact intersection testing
	//

	vec2d verts1[4];
	vec2d verts2[4];

	for( int i = 0; i < 4; ++i )
	{
		verts1[i] = GetVertex(i);
		verts2[i] = pObj->GetVertex(i);
	}

	float ax1, ay1, ax2, ay2;
	float bx1, by1, bx2, by2;
	float ax, ay, bx, by;

	float det, ta, tb;

	vec2d pt[2];
	vec2d ns[2];
	int icount = 0;

	for( int i = 0; i < 4; ++i )
	for( int j = 0; j < 4; ++j )
	{
		ax1 = verts1[i].x;
		ay1 = verts1[i].y;
		ax2 = verts1[(i+1)&3].x;
		ay2 = verts1[(i+1)&3].y;
		bx1 = verts2[j].x;
		by1 = verts2[j].y;
		bx2 = verts2[(j+1)&3].x;
		by2 = verts2[(j+1)&3].y;

		ax = ax2 - ax1;
		ay = ay2 - ay1;
		bx = bx2 - bx1;
		by = by2 - by1;

		if( fabsf(det = ay*bx - ax*by) > 1e-7 )
		{
			ta = ( by*(ax1-bx1) + bx*(by1-ay1) ) / det;
			if( ta < 0 || ta > 1 )
				continue;

			tb = ( ay*(ax1-bx1) + ax*(by1-ay1) ) / det;
			if( tb < 0 || tb > 1 )
				continue;

			origin.x = ax1 + ax * ta;
			origin.y = ay1 + ay * ta;

			pt[icount++] = origin;

			if( 2 == icount )
			{
				origin = (pt[0] + pt[1]) * 0.5f;

				normal.x = pt[1].y - pt[0].y;
				normal.y = pt[0].x - pt[1].x;

				if( normal.x * (origin.x - GetPos().x) +
					normal.y * (origin.y - GetPos().y) > 0 )
				{
					normal.x = -normal.x;
					normal.y = -normal.y;
				}


				float len = normal.Length();
				if( len < 1e-7 )
				{
					return false;
				}

				normal /= len;
				return true;
			}

			if( fabsf(ta - 0.5f) < fabsf(tb - 0.5f) )
			{
				normal.x = -ay;
				normal.y =  ax;
			}
			else
			{
				normal.x =  by;
				normal.y = -bx;
			}
			normal.Normalize();
		}
		else
		{
		}
	}

	return (icount > 0);
}

void GC_RigidBodyDynamic::TimeStepFixed(float dt)
{
	vec2d dx = _lv * dt;
	float da = _av * dt;


	//
	// защита от пролетания снарядов через объект
	//

	AddRef();
	{
		float s = sinf(da);
		float c = cosf(da) - 1;
		OBJECT_LIST::safe_iterator it = g_level->projectiles.safe_begin();
		while( it != g_level->projectiles.end() )
		{
			GC_Projectile* pProj = static_cast<GC_Projectile*>(*it);

			vec2d delta, tmp = pProj->GetPos() - GetPos();
			delta.x = tmp.x * c - tmp.y * s - dx.x;
			delta.y = tmp.x * s - tmp.y * c - dx.y;

			float dl_sq = delta.Square();
			if( dl_sq < 1 ) delta /= sqrtf(dl_sq);

			pProj->SpecialTrace(this, delta);
			++it;

			if( IsKilled() )
			{
				Release();
				return;
			}
		}
	}
	Release();



	MoveTo(GetPos() + dx);
	_angle = fmodf(_angle + da, PI2);
	if( _angle < 0 ) _angle += PI2;


	_direction = vec2d(_angle);
	SetRotation(_angle);

	//------------------------------------

	apply_external_forces(dt);

	//------------------------------------
	// friction

	vec2d dir_y(_direction.y, -_direction.x);

	float vx = _lv.x * _direction.x + _lv.y * _direction.y;
	float vy = _lv.x * _direction.y - _lv.y * _direction.x;

	if( vx > 0 )
		vx = __max(0, vx - _Nx * dt);
	else
		vx = __min(0, vx + _Nx * dt);
	vx *= expf(-_Mx * dt);

	if( vy > 0 )
		vy = __max(0, vy - _Ny * dt);
	else
		vy = __min(0, vy + _Ny * dt);
	vy *= expf(-_My * dt);

	_lv = _direction * vx + dir_y * vy;


	if( _av > 0 )
		_av = __max(0, _av - _Nw * dt);
	else
		_av = __min(0, _av + _Nw * dt);
	_av *= expf(-_Mw * dt);


	//------------------------------------
	// collisions

	std::vector<OBJECT_LIST*> receive;
	g_level->grid_rigid_s.OverlapCircle(receive,
		GetPos().x / LOCATION_SIZE, GetPos().y / LOCATION_SIZE, 0);

	g_level->grid_water.OverlapCircle(receive,
		GetPos().x / LOCATION_SIZE, GetPos().y / LOCATION_SIZE, 0);

	std::vector<OBJECT_LIST*>::iterator rit = receive.begin();

	Contact c;
	c.total_np = 0;
	c.total_tp = 0;
	c.obj1_d   = this;

	for( ; rit != receive.end(); ++rit )
	{
		OBJECT_LIST::iterator it = (*rit)->begin();
		for( ; it != (*rit)->end(); ++it )
		{
			GC_RigidBodyStatic *object = (GC_RigidBodyStatic *) (*it);
			if( this == object ) continue;

//			if( object->parity() != _glob_parity )
			{
				if( Intersect(object, c.o, c.n) )
				{
					c.t.x =  c.n.y;
					c.t.y = -c.n.x;
					c.obj2_s = object;
					c.obj2_d = dynamic_cast<GC_RigidBodyDynamic*>(object);

					c.obj1_d->AddRef();
					c.obj2_s->AddRef();

                    _contacts.push_back(c);
				}
			}
		}
	}
}

float GC_RigidBodyDynamic::geta_s(const vec2d &n, const vec2d &c,
								  const GC_RigidBodyStatic *obj) const
{
	float k1 = n.x*(c.y-GetPos().y) - n.y*(c.x-GetPos().x);
	return (float)
	(
		2 * ( _av * k1 - n.y*_lv.y - n.x*_lv.x ) /
		( k1*k1*_inv_i + n.Square() * _inv_m )
	);
}

float GC_RigidBodyDynamic::geta_d(const vec2d &n, const vec2d &c,
								  const GC_RigidBodyDynamic *obj) const
{
	float k1 = n.x*(c.y-GetPos().y) - n.y*(c.x-GetPos().x);
	float k2 = n.y*(c.x-obj->GetPos().x) - n.x*(c.y-obj->GetPos().y);
	return (float)
	(
		2 * ( n.y*(obj->_lv.y-_lv.y) + n.x*(obj->_lv.x-_lv.x) + _av*k1 + obj->_av*k2 ) /
		    ( k1*k1*_inv_i + k2*k2*obj->_inv_i + n.Square() * (_inv_m+obj->_inv_m) )
	);
}

void GC_RigidBodyDynamic::ProcessResponse(float dt)
{
	for( int i = 0; i < 128; i++ )
	{
		for( ContactList::iterator it = _contacts.begin(); it != _contacts.end(); ++it )
		{
			if( it->obj1_d->IsKilled() || it->obj2_s->IsKilled() ) continue;

			float a;
			if( it->obj2_d )
				a = 0.65f * it->obj1_d->geta_d(it->n, it->o, it->obj2_d );
			else
				a = 0.65f * it->obj1_d->geta_s(it->n, it->o, it->obj2_s );

			if( a >= 0 )
			{
				a = __max(0.01f * (float) (i>>2), a);
				it->total_np += a;

				float nd = it->total_np/60;

				if( nd > 3 )
				{
					if( it->obj2_d )
					{
						it->obj1_d->TakeDamage(a/60 * it->obj2_d->_percussion *
							it->obj1_d->_fragility, it->o, it->obj2_d);
						it->obj2_d->TakeDamage(a/60 * it->obj1_d->_percussion *
							it->obj2_d->_fragility, it->o, it->obj1_d);
					}
					else
					{
						it->obj1_d->TakeDamage(a/60 * it->obj1_d->_fragility, it->o, it->obj1_d);
						it->obj2_s->TakeDamage(a/60 * it->obj1_d->_percussion, it->o, it->obj1_d);
					}
				}

				if( it->obj1_d->IsKilled() || it->obj2_s->IsKilled() )
					a *= 0.1f;

				vec2d delta_p = it->n * a;
				it->obj1_d->impulse(it->o, delta_p);
				if( it->obj2_d ) it->obj2_d->impulse(it->o, -delta_p);


				//
				// трение
				//

				const float N = 0.1f;

				float maxb;

				if( it->obj2_d )
					maxb = 0.5f * it->obj1_d->geta_d(it->t, it->o, it->obj2_d );
				else
					maxb = 0.5f * it->obj1_d->geta_s(it->t, it->o, it->obj2_s );

				float signb = maxb > 0 ? 1.0f : -1.0f;
				if( (maxb = fabsf(maxb)) > 0 )
				{
					float b = __min(maxb, a*N);
					it->total_tp += b;
					delta_p = it->t * b * signb;
					it->obj1_d->impulse(it->o,  delta_p);
					if( it->obj2_d ) it->obj2_d->impulse(it->o, -delta_p);
				}
			}
		}
	}

	for( ContactList::iterator it = _contacts.begin(); it != _contacts.end(); ++it )
	{
		float nd = (it->total_np + it->total_tp)/60;

		if( nd > 3 )
		{
			if( nd > 10 )
				PLAY(SND_Impact2, it->o);
			else
				PLAY(SND_Impact1, it->o);
		}
		else
		if( it->total_tp > 10 )
		{
			PLAY(SND_Slide1, it->o);
		}

        /////////////////////////////////////
		it->obj1_d->Release();
		it->obj2_s->Release();
	}

	_contacts.clear();
}

void GC_RigidBodyDynamic::impulse(const vec2d &origin, const vec2d &impulse)
{
	_lv += impulse * _inv_m;
	_av += ((origin.x-GetPos().x)*impulse.y-(origin.y-GetPos().y)*impulse.x) * _inv_i;
}

void GC_RigidBodyDynamic::apply_external_forces(float dt)
{
	_lv += (_external_force * dt + _external_impulse) * _inv_m;
	_av += (_external_momentum * dt + _external_torque) * _inv_i;
	_external_force.Zero();
	_external_impulse.Zero();
	_external_momentum = 0;
	_external_torque = 0;
}

void GC_RigidBodyDynamic::ApplyMomentum(float momentum)
{
	_external_momentum += momentum;
}

void GC_RigidBodyDynamic::ApplyForce(const vec2d &force)
{
	_external_force += force;
}

void GC_RigidBodyDynamic::ApplyForce(const vec2d &force, const vec2d &origin)
{
	_external_force += force;
	_external_momentum += (origin.x-GetPos().x)*force.y-(origin.y-GetPos().y)*force.x;
}

void GC_RigidBodyDynamic::ApplyImpulse(const vec2d &impulse, const vec2d &origin)
{
	_external_impulse += impulse;
	_external_torque  += (origin.x-GetPos().x)*impulse.y-(origin.y-GetPos().y)*impulse.x;
}

void GC_RigidBodyDynamic::ApplyImpulse(const vec2d &impulse)
{
	_external_impulse += impulse;
}

void GC_RigidBodyDynamic::ApplyTorque(float torque)
{
	_external_torque  += torque;
}

void GC_RigidBodyDynamic::SetBodyAngle(float a)
{
	_angle = a;
	_direction = vec2d(_angle);
	SetRotation(_angle);
}

float GC_RigidBodyDynamic::Energy() const
{
	float e = 0;
	if( _inv_i != 0 ) e  = _av*_av / _inv_i;
	if( _inv_m != 0 ) e += _lv.Square() / _inv_m;
	return e;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

// RigidBody.cpp

#include "stdafx.h"

#include "RigidBody.h"

#include "level.h"
#include "functions.h"
#include "script.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"
#include "config/Config.h"

#include "Sound.h"
#include "Particles.h"

///////////////////////////////////////////////////////////////////////////////

GC_RigidBodyStatic::GC_RigidBodyStatic()
  : GC_2dSprite()
  , _health(1)
  , _health_max(1)
  , _radius(0)
  , _width(0)
  , _length(0)
{
	AddContext(&g_level->grid_rigid_s);
}

GC_RigidBodyStatic::GC_RigidBodyStatic(FromFile)
  : GC_2dSprite(FromFile())
  , _radius(0) // for proper handling of bad save files
{
}

bool GC_RigidBodyStatic::CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection,
                                         float &outWhere, vec2d &outNormal)
{
	assert(!_isnan(lineCenter.x) && _finite(lineCenter.x));
	assert(!_isnan(lineCenter.y) && _finite(lineCenter.y));
	assert(!_isnan(lineDirection.x) && _finite(lineDirection.x));
	assert(!_isnan(lineDirection.y) && _finite(lineDirection.y));

	float lineProjL = Vec2dDot(lineDirection, GetDirection());
	float lineProjW = Vec2dCross(lineDirection, GetDirection());
	float lineProjL_abs = fabs(lineProjL);
	float lineProjW_abs = fabs(lineProjW);

	//
	// project box to lineDirection axis
	//

	vec2d delta = GetPos() - lineCenter;
	float halfProjLine = lineProjL_abs * GetHalfWidth() + lineProjW_abs * GetHalfLength();
	if( fabs(Vec2dCross(delta, lineDirection)) > halfProjLine )
		return false;

	//
	// project lineDirection to box axes
	//

	float deltaDotDir = Vec2dDot(delta, GetDirection());
	if( fabs(deltaDotDir) > lineProjL_abs / 2 + GetHalfLength() )
		return false;

	float deltaCrossDir = Vec2dCross(delta, GetDirection());
	if( fabs(deltaCrossDir) > lineProjW_abs / 2 + GetHalfWidth() )
		return false;

	//
	// calc intersection point and normal
	//

	float signW = lineProjW > 0 ? 1.0f : -1.0f;
	float signL = lineProjL > 0 ? 1.0f : -1.0f;

	float b1 = (deltaCrossDir * signW - GetHalfWidth());
	float b2 = (deltaDotDir * signL - GetHalfLength());
	if( b1 * lineProjL_abs > b2 * lineProjW_abs )
	{
		outWhere = lineProjW_abs > 0 ? b1 / lineProjW_abs : 0;
		outNormal = lineProjW > 0 ?
			vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
	}
	else
	{
		outWhere = lineProjL_abs > 0 ? b2 / lineProjL_abs : 0;
		outNormal = lineProjL > 0 ? -GetDirection() : GetDirection();
	}

	return true;
}

bool GC_RigidBodyStatic::CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection,
                                         float &outWhere, vec2d &outNormal)
{
	assert(!_isnan(rectHalfSize.x) && _finite(rectHalfSize.x));
	assert(!_isnan(rectHalfSize.y) && _finite(rectHalfSize.y));
	assert(!_isnan(rectCenter.x) && _finite(rectCenter.x));
	assert(!_isnan(rectCenter.y) && _finite(rectCenter.y));
	assert(!_isnan(rectDirection.x) && _finite(rectDirection.x));
	assert(!_isnan(rectDirection.y) && _finite(rectDirection.y));

	vec2d delta = GetPos() - rectCenter;

	//
	// project this to rectDirection axis
	//

	float projL = Vec2dDot(rectDirection, GetDirection());
	float projW = Vec2dCross(rectDirection, GetDirection());
	float projL_abs = fabs(projL);
	float projW_abs = fabs(projW);

	float halfProjRectL = projL_abs * GetHalfWidth() + projW_abs * GetHalfLength();
	if( fabs(Vec2dCross(delta, rectDirection)) > rectHalfSize.y + halfProjRectL )
		return false;

	//
	// project this to rectDirection|- axis
	//

	float halfProjRectW = projW_abs * GetHalfWidth() + projL_abs * GetHalfLength();
	float deltaDotDir = Vec2dDot(delta, GetDirection());
	if( fabs(deltaDotDir) > rectHalfSize.x + halfProjRectW )
		return false;


	//
	// project rectDirection to this axes
	//

	if( fabs(deltaDotDir) > projL_abs * rectHalfSize.x + projW_abs * rectHalfSize.y + GetHalfLength() )
		return false;

	float deltaCrossDir = Vec2dCross(delta, GetDirection());
	if( fabs(deltaCrossDir) > projW_abs * rectHalfSize.x + projL_abs * rectHalfSize.y + GetHalfWidth() )
		return false;



	//
	// calc intersection point and normal
	//

	float signW = projW > 0 ? 1.0f : -1.0f;
	float signL = projL > 0 ? 1.0f : -1.0f;

	float b1 = (deltaCrossDir * signW - GetHalfWidth());
	float b2 = (deltaDotDir * signL - GetHalfLength());
	if( b1 * projL_abs > b2 * projW_abs )
	{
		outWhere = projW_abs > 0 ? b1 / projW_abs : 0;
		outNormal = projW > 0 ?
			vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
	}
	else
	{
		outWhere = projL_abs > 0 ? b2 / projL_abs : 0;
		outNormal = projL > 0 ? -GetDirection() : GetDirection();
	}

	return true;
}

void GC_RigidBodyStatic::SetHealth(float cur, float max)
{
	assert(cur <= max);
	_health = cur;
	_health_max = max;
}

void GC_RigidBodyStatic::SetHealthCur(float hp)
{
	assert(hp <= _health_max);
	_health = hp;
}

void GC_RigidBodyStatic::SetHealthMax(float hp)
{
	assert(hp >= _health);
	_health_max = hp;
}

void GC_RigidBodyStatic::OnDestroy()
{
	PulseNotify(NOTIFY_RIGIDBODY_DESTROY);
	if( !_scriptOnDestroy.empty() )
	{
		script_exec(g_env.L, _scriptOnDestroy.c_str());
	}
}

bool GC_RigidBodyStatic::TakeDamage(float damage, const vec2d &hit, GC_Player *from)
{
	assert(!IsKilled());

	if( CheckFlags(GC_FLAG_RBSTATIC_DESTROYED) )
	{
		return true;
	}

	if( _health_max > 0 )
	{
		SetHealthCur(GetHealth() - damage);

		if( !_scriptOnDamage.empty() )
		{
			SafePtr<GC_Object> refHolder(this);
			script_exec(g_env.L, _scriptOnDamage.c_str());
			if( IsKilled() )
			{
				return true;
			}
		}

		if( GetHealth() <= 0 )
		{
			SafePtr<GC_Object> refHolder(this);
			SetFlags(GC_FLAG_RBSTATIC_DESTROYED, true);
			OnDestroy();
			Kill();
			return true;
		}
	}
	return false;
}

void GC_RigidBodyStatic::AlignToTexture()
{
	SetSize(GetSpriteHeight(), GetSpriteWidth());
}

void GC_RigidBodyStatic::SetSize(float width, float length)
{
	_width = width;
	_length = length;
	_radius = sqrt(width*width + length*length) / 2;
}

void GC_RigidBodyStatic::MapExchange(MapFile &f)
{
	GC_2dSprite::MapExchange(f);

	MAP_EXCHANGE_FLOAT(  health,     _health,     GetDefaultHealth());
	MAP_EXCHANGE_FLOAT(  health_max, _health_max, GetDefaultHealth());
	MAP_EXCHANGE_STRING( on_destroy, _scriptOnDestroy, "");
	MAP_EXCHANGE_STRING( on_damage,  _scriptOnDamage,  "");

	if( f.loading() )
	{
		_health = __min(_health, _health_max);
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
	f.Serialize(_width);
	f.Serialize(_length);

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

	assert(FALSE);
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

bool GC_Wall::CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection, float &outWhere, vec2d &outNormal)
{
	assert(!_isnan(lineCenter.x) && _finite(lineCenter.x));
	assert(!_isnan(lineCenter.y) && _finite(lineCenter.y));
	assert(!_isnan(lineDirection.x) && _finite(lineDirection.x));
	assert(!_isnan(lineDirection.y) && _finite(lineDirection.y));

	unsigned int corner = GetCorner();
	if( corner )
	{
		vec2d delta = GetPos() - lineCenter;

		//
		// project lineDirection to triangle axes
		//

		float lineProjL = Vec2dDot(lineDirection, GetDirection());
		float lineProjL_abs = fabs(lineProjL);
		float deltaDotDir = Vec2dDot(delta, GetDirection());
		if( fabs(deltaDotDir) > lineProjL_abs / 2 + GetHalfLength() )
			return false;

		float lineProjW = Vec2dCross(lineDirection, GetDirection());
		float lineProjW_abs = fabs(lineProjW);
		float deltaCrossDir = Vec2dCross(delta, GetDirection());
		if( fabs(deltaCrossDir) > lineProjW_abs / 2 + GetHalfWidth() )
			return false;

		static const vec2d angles[4] = {vec2d(5*PI4), vec2d(7*PI4), vec2d(PI4), vec2d(3*PI4)};
		vec2d diagonal = Vec2dAddDirection(angles[corner-1], GetDirection());
		float halfDiag = sqrt(GetHalfWidth()*GetHalfWidth() + GetHalfLength()*GetHalfLength());
		float lineProjD = Vec2dDot(lineDirection, diagonal);
		float lineProjD_abs = fabs(lineProjD);
		float deltaDotDiagonal = Vec2dDot(delta, diagonal);
		if( fabs(deltaDotDiagonal + halfDiag / 2) > lineProjD_abs / 2 + halfDiag / 2 )
			return false;

		//
		// project triangle to lineDirection axis
		//

		float halfProjLineW = lineProjL_abs * GetHalfWidth();
		float halfProjLineL = lineProjW_abs * GetHalfLength();
		float halfProjLineD = lineProjD_abs * halfDiag;
		float halfProjLine = std::max(halfProjLineW, std::max(halfProjLineL, halfProjLineD));
		float dirLen = lineDirection.len();
		vec2d offset = diagonal * (halfProjLineW + halfProjLineL - halfProjLine);
		if( fabs(Vec2dCross(delta * dirLen + offset, lineDirection)) > halfProjLine * dirLen )
			return false;

		//
		// calc intersection point and normal
		//

		float signW = lineProjW > 0 ? 1.0f : -1.0f;
		float signL = lineProjL > 0 ? 1.0f : -1.0f;

		float bW = deltaCrossDir * signW - GetHalfWidth();
		float bL = deltaDotDir * signL - GetHalfLength();
		float bD = deltaDotDiagonal;

		if( bW * lineProjL_abs > bL * lineProjW_abs && bW * lineProjD_abs > bD * lineProjW_abs )
		{
			outWhere = lineProjW_abs > 0 ? bW / lineProjW_abs : 0;
			outNormal = lineProjW > 0 ?
				vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
		}
		else if( bL * lineProjD_abs > bD * lineProjL_abs )
		{
			outWhere = lineProjL_abs > 0 ? bL / lineProjL_abs : 0;
			outNormal = lineProjL > 0 ? -GetDirection() : GetDirection();
		}
		else
		{
			outWhere = lineProjD_abs > 0 ? bD / lineProjD_abs : 0;
			outNormal = -diagonal;
		}

		return true;
	}
	else
	{
		return __super::CollideWithLine(lineCenter, lineDirection, outWhere, outNormal);
	}
}


void GC_Wall::Kill()
{
	SetCorner(0);
	GC_RigidBodyStatic::Kill();
}

void GC_Wall::MapExchange(MapFile &f)
{
	GC_RigidBodyStatic::MapExchange(f);
	int corner = GetCorner();
	int style = GetStyle();
	MAP_EXCHANGE_INT(corner, corner, 0);
	MAP_EXCHANGE_INT(style, style, 0);

	if( f.loading() )
	{
		SetCorner(corner % 5);
		SetStyle(style % 4);
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
			switch( GetCorner() )
			{
			case 0:
				break;
			case 1:
				x = int(p.x+1);
				y = int(p.y+1);
				break;
			case 2:
				x = int(p.x);
				y = int(p.y + 1);
				break;
			case 3:
				x = int(p.x + 1);
				y = int(p.y);
				break;
			case 4:
				x = int(p.x);
				y = int(p.y);
				break;
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

	if( g_conf.g_particles.Get() )
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

bool GC_Wall::TakeDamage(float damage, const vec2d &hit, GC_Player *from)
{
	if( !GC_RigidBodyStatic::TakeDamage(damage, hit, from) && GetHealthMax() > 0 )
	{
		SetFrame((GetFrameCount()-1)-int((float)(GetFrameCount()-1)*GetHealth()/GetHealthMax()));
		if( g_conf.g_particles.Get() && damage >= DAMAGE_BULLET )
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

void GC_Wall::SetCorner(unsigned int index) // 0 means normal view
{
	assert(index < 5);
	static const DWORD flags[] = {
		0,
		GC_FLAG_WALL_CORNER_1,
		GC_FLAG_WALL_CORNER_2,
		GC_FLAG_WALL_CORNER_3,
		GC_FLAG_WALL_CORNER_4
	};

	vec2d p = GetPos() / CELL_SIZE;
	if( CheckFlags(GC_FLAG_WALL_CORNER_ALL) )
	{
		int x;
		int y;
		switch( GetCorner() )
		{
		case 0:
			break;
		case 1:
			x = int(p.x+1);
			y = int(p.y+1);
			break;
		case 2:
			x = int(p.x);
			y = int(p.y + 1);
			break;
		case 3:
			x = int(p.x + 1);
			y = int(p.y);
			break;
		case 4:
			x = int(p.x);
			y = int(p.y);
			break;
		}
		g_level->_field(x, y).AddObject(this);
	}

	SetFlags(GC_FLAG_WALL_CORNER_ALL, false);
	SetFlags(flags[index], true);

	SetTexture(GetCornerTexture(index));
	AlignToTexture();

	if( CheckFlags(GC_FLAG_WALL_CORNER_ALL) )
	{
		int x;
		int y;
		switch( GetCorner() )
		{
		case 0:
			break;
		case 1:
			x = int(p.x+1);
			y = int(p.y+1);
			break;
		case 2:
			x = int(p.x);
			y = int(p.y + 1);
			break;
		case 3:
			x = int(p.x + 1);
			y = int(p.y);
			break;
		case 4:
			x = int(p.x);
			y = int(p.y);
			break;
		}
		g_level->_field(x, y).RemoveObject(this);
		if( 0 == x || 0 == y || g_level->_field.GetX() - 1 == x || g_level->_field.GetX() - 1 == y )
		{
			g_level->_field(x, y)._prop = 0xFF;
		}
	}
}

unsigned int GC_Wall::GetCorner(void) const
{
	switch( GetFlags() & GC_FLAG_WALL_CORNER_ALL )
	{
	default: assert(false);
	case 0:                     return 0;
	case GC_FLAG_WALL_CORNER_1: return 1;
	case GC_FLAG_WALL_CORNER_2: return 2;
	case GC_FLAG_WALL_CORNER_3: return 3;
	case GC_FLAG_WALL_CORNER_4: return 4;
	}
}

void GC_Wall::SetStyle(int style) // 0-3
{
	assert(style >= 0 && style < 4);
	static const int s[] = 
	{
		0, 
		GC_FLAG_WALL_STYLE_BIT_0,
		GC_FLAG_WALL_STYLE_BIT_1,
		GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1
	};
	SetFlags(GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1, false);
	SetFlags(s[style], true);
}

int GC_Wall::GetStyle() const
{
	switch( GetFlags() & (GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1) )
	{
	case GC_FLAG_WALL_STYLE_BIT_0:
		return 1;
	case GC_FLAG_WALL_STYLE_BIT_1:
		return 2;
	case GC_FLAG_WALL_STYLE_BIT_0|GC_FLAG_WALL_STYLE_BIT_1:
		return 3;
	}
	return 0;
}

const char* GC_Wall::GetCornerTexture(int i)
{
	assert(i >=0 && i < 5);
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
  , _propStyle(  ObjectProperty::TYPE_INTEGER,   "style"   )
{
	_propCorner.SetIntRange(0, 4);
	_propStyle.SetIntRange(0, 3);
}

int GC_Wall::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_Wall::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propCorner;
	case 1: return &_propStyle;
	}

	assert(FALSE);
	return NULL;
}

void GC_Wall::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Wall *tmp = static_cast<GC_Wall *>(GetObject());

	if( applyToObject )
	{
		tmp->SetCorner(_propCorner.GetIntValue());
		tmp->SetStyle(_propStyle.GetIntValue());
	}
	else
	{
		_propCorner.SetIntValue(tmp->GetCorner());
		_propStyle.SetIntValue(tmp->GetStyle());
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
	AlignToTexture();

	SetFrame(rand() % GetFrameCount());

	g_level->_field.ProcessObject(this, true);
}

bool GC_Wall_Concrete::TakeDamage(float damage, const vec2d &hit, GC_Player *from)
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
	assert(i >=0 && i < 5);
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
  , _tile(0)
{
	AddContext( &g_level->grid_water );

	SetZ(Z_WATER);

	SetTexture("water");
	AlignToTexture();

	MoveTo( vec2d(xPos, yPos) );
	SetFrame(4);

	UpdateTile(true);

	SetFlags(GC_FLAG_RBSTATIC_TRACE0, true);

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

	PtrList<ObjectList> receive;
	g_level->grid_water.OverlapRect(receive, frect);
	///////////////////////////////////////////////////
	PtrList<ObjectList>::iterator rit = receive.begin();
	for( ; rit != receive.end(); ++rit )
	{
		ObjectList::iterator it = (*rit)->begin();
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

void GC_Water::Draw() const
{
	static const float dx[8]   = { 32, 32,  0,-32,-32,-32,  0, 32 };
	static const float dy[8]   = {  0, 32, 32, 32,  0,-32,-32,-32 };
	static const int frames[8] = {  5,  8,  7,  6,  3,  0,  1,  2 };

	vec2d pos = GetPosPredicted();

	for( int i = 0; i < 8; ++i )
	{
		if( 0 == (_tile & (1 << i)) )
		{
			g_texman->DrawSprite(GetTexture(), frames[i], 0xffffffff, pos.x + dx[i], pos.y + dy[i], GetDirection());
		}
	}
	g_texman->DrawSprite(GetTexture(), 4, 0xffffffff, pos.x, pos.y, GetDirection());
}

void GC_Water::SetTile(char nTile, bool value)
{
	assert(0 <= nTile && nTile < 8);

	if( value )
		_tile |= (1 << nTile);
	else
		_tile &= ~(1 << nTile);
}

bool GC_Water::TakeDamage(float damage, const vec2d &hit, GC_Player *from)
{
	return false;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

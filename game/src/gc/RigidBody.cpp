#include "TypeReg.h"
#include "inc/gc/RigidBody.h"
#include "inc/gc/Player.h"
#include "inc/gc/Vehicle.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/WorldEvents.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>
#include <cfloat>

IMPLEMENT_GRID_MEMBER(GC_RigidBodyStatic, grid_rigid_s);

GC_RigidBodyStatic::GC_RigidBodyStatic(vec2d pos)
  : GC_Actor(pos)
  , _health(1)
  , _health_max(1)
  , _radius(0)
  , _width(0)
  , _length(0)
{
}

GC_RigidBodyStatic::GC_RigidBodyStatic(FromFile)
  : GC_Actor(FromFile())
{
}

GC_RigidBodyStatic::~GC_RigidBodyStatic()
{
}

void GC_RigidBodyStatic::Init(World &world)
{
	GC_Actor::Init(world);
	if( GetPassability() > 0 )
		world._field.ProcessObject(this, true);
}

void GC_RigidBodyStatic::Kill(World &world)
{
	if( GetPassability() > 0 )
		world._field.ProcessObject(this, false);
    GC_Actor::Kill(world);
}

void GC_RigidBodyStatic::MoveTo(World &world, const vec2d &pos)
{
	if( GetPassability() > 0 )
		world._field.ProcessObject(this, false);

    GC_Actor::MoveTo(world, pos);

    if( GetPassability() > 0 )
		world._field.ProcessObject(this, true);
}

bool GC_RigidBodyStatic::CollideWithLine(const vec2d &lineCenter, const vec2d &lineDirection,
                                         vec2d &outEnterNormal, float &outEnter, float &outExit) const
{
	assert(!std::isnan(lineCenter.x) && std::isfinite(lineCenter.x));
	assert(!std::isnan(lineCenter.y) && std::isfinite(lineCenter.y));
	assert(!std::isnan(lineDirection.x) && std::isfinite(lineDirection.x));
	assert(!std::isnan(lineDirection.y) && std::isfinite(lineDirection.y));

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

	float b1 = deltaCrossDir * signW - GetHalfWidth();
	float b2 = deltaDotDir * signL - GetHalfLength();
	if( b1 * lineProjL_abs > b2 * lineProjW_abs )
	{
		outEnter = lineProjW_abs > std::numeric_limits<float>::epsilon() ? b1 / lineProjW_abs : 0;
		outEnterNormal = lineProjW > 0 ?
			vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
	}
	else
	{
		outEnter = lineProjL_abs > std::numeric_limits<float>::epsilon() ? b2 / lineProjL_abs : 0;
		outEnterNormal = lineProjL > 0 ? -GetDirection() : GetDirection();
	}

	float b1e = deltaCrossDir * signW + GetHalfWidth();
	float b2e = deltaDotDir * signL + GetHalfLength();
	if( b1e * lineProjL_abs < b2e * lineProjW_abs )
	{
		outExit = lineProjW_abs > std::numeric_limits<float>::epsilon() ? b1e / lineProjW_abs : 0;
	}
	else
	{
		outExit = lineProjL_abs > std::numeric_limits<float>::epsilon() ? b2e / lineProjL_abs : 0;
	}

	return true;
}

bool GC_RigidBodyStatic::CollideWithRect(const vec2d &rectHalfSize, const vec2d &rectCenter, const vec2d &rectDirection,
                                         vec2d &outWhere, vec2d &outNormal, float &outDepth) const
{
	assert(!std::isnan(rectHalfSize.x) && std::isfinite(rectHalfSize.x));
	assert(!std::isnan(rectHalfSize.y) && std::isfinite(rectHalfSize.y));
	assert(!std::isnan(rectCenter.x) && std::isfinite(rectCenter.x));
	assert(!std::isnan(rectCenter.y) && std::isfinite(rectCenter.y));
	assert(!std::isnan(rectDirection.x) && std::isfinite(rectDirection.x));
	assert(!std::isnan(rectDirection.y) && std::isfinite(rectDirection.y));

	vec2d delta = GetPos() - rectCenter;
	float depth[4];

	//
	// project this to rectDirection axes
	//

	float projL = Vec2dDot(rectDirection, GetDirection());
	float projW = Vec2dCross(rectDirection, GetDirection());
	float projL_abs = fabs(projL);
	float projW_abs = fabs(projW);

	float halfProjRectL = projL_abs * GetHalfWidth() + projW_abs * GetHalfLength();
	float deltaCrossRectDir = Vec2dCross(delta, rectDirection);
	depth[0] = rectHalfSize.y + halfProjRectL - fabs(deltaCrossRectDir);
	if( depth[0] < 0 ) return false;


	float halfProjRectW = projW_abs * GetHalfWidth() + projL_abs * GetHalfLength();
	float deltaDotRectDir = Vec2dDot(delta, rectDirection);
	depth[1] = rectHalfSize.x + halfProjRectW - fabs(deltaDotRectDir);
	if( depth[1] < 0 ) return false;


	//
	// project rectDirection to this axes
	//

	float halfProjThisL = projW_abs * rectHalfSize.x + projL_abs * rectHalfSize.y;
	float deltaCrossDir = Vec2dCross(delta, GetDirection());
	depth[2] = GetHalfWidth() + halfProjThisL - fabs(deltaCrossDir);
	if( depth[2] < 0 ) return false;

	float halfProjThisW = projL_abs * rectHalfSize.x + projW_abs * rectHalfSize.y;
	float deltaDotDir = Vec2dDot(delta, GetDirection());
	depth[3] = GetHalfLength() + halfProjThisW - fabs(deltaDotDir);
	if( depth[3] < 0 ) return false;


	//
	// calculate penetration depth
	//

	outDepth = depth[0];
	unsigned int mdIndex = 0;
	for( int i = 1; i < 4; ++i )
	{
		if( depth[i] < outDepth )
		{
			outDepth = depth[i];
			mdIndex = i;
		}
	}

	// calc normal
	switch( mdIndex )
	{
	case 0:
		outNormal = deltaCrossRectDir > 0 ?
			vec2d(-rectDirection.y, rectDirection.x) : vec2d(rectDirection.y, -rectDirection.x);
		break;
	case 1:
		outNormal = deltaDotRectDir > 0 ? -rectDirection : rectDirection;
		break;
	case 2:
		outNormal = deltaCrossDir > 0 ?
			vec2d(-GetDirection().y, GetDirection().x) : vec2d(GetDirection().y, -GetDirection().x);
		break;
	case 3:
		outNormal = deltaDotDir > 0 ? -GetDirection() : GetDirection();
		break;
	default:
		assert(false);
	}

//	int group = mdIndex/2;
//	int ingrp = mdIndex%2;
//	int neighbor = group*2 + 1 - ingrp


	// contact manifold

	float xx, xy, yx, yy;
	float sign;
	vec2d center;
	if( mdIndex < 2 )
	{
		xx = GetHalfLength()*GetDirection().x;
		xy = GetHalfLength()*GetDirection().y;
		yx = GetHalfWidth()*GetDirection().x;
		yy = GetHalfWidth()*GetDirection().y;
		center = GetPos();
		sign = -1;
	}
	else
	{
		xx = rectHalfSize.x*rectDirection.x;
		xy = rectHalfSize.x*rectDirection.y;
		yx = rectHalfSize.y*rectDirection.x;
		yy = rectHalfSize.y*rectDirection.y;
		center = rectCenter;
		sign = 1;
	}

	vec2d v[4] =
	{
		vec2d(xx - yy, yx + xy),
		vec2d(xx + yy, -yx + xy),
		vec2d(-xx - yy, yx - xy),
		vec2d(-xx + yy, -yx - xy)
	};
	unsigned int idx;
	float mindot = FLT_MAX;
	for( unsigned int i = 0; i < 4; ++i )
	{
		float dot = sign*Vec2dDot(outNormal, v[i]);
		if( mindot > dot )
		{
			mindot = dot;
			idx = i;
		}
	}

	if( projL_abs < 1e-3 || projW_abs < 1e-3 )
	{
		// for edge-edge collision find second closest point
		mindot = FLT_MAX;
		unsigned int idx2;
		for( unsigned int i = 0; i < 4; ++i )
		{
			if( i != idx )
			{
				float dot = sign*Vec2dDot(outNormal, v[i]);
				if( mindot > dot )
				{
					mindot = dot;
					idx2 = i;
				}
			}
		}

		outWhere = center + (v[idx] + v[idx2]) * 0.5f;
	}
	else
	{
		outWhere = center + v[idx];
	}

	return true;
}

void GC_RigidBodyStatic::SetHealth(float cur, float max)
{
	assert(cur <= max);
	_health = cur;
	_health_max = max;
}

void GC_RigidBodyStatic::SetHealth(float hp)
{
	assert(hp <= _health_max);
	_health = hp;
}

void GC_RigidBodyStatic::SetHealthMax(float hp)
{
	assert(hp >= _health);
	_health_max = hp;
}

void GC_RigidBodyStatic::OnDestroy(World &world, const DamageDesc &dd)
{
	for( auto ls: world.eGC_RigidBodyStatic._listeners )
		ls->OnDestroy(*this, dd);
}

void GC_RigidBodyStatic::OnDamage(World &world, DamageDesc &dd)
{
}

void GC_RigidBodyStatic::TakeDamage(World &world, DamageDesc dd)
{
	if( CheckFlags(GC_FLAG_RBSTATIC_DESTROYED) )
	{
		return;
	}

	ObjPtr<GC_Object> watch(this);
	for( auto ls: world.eGC_RigidBodyStatic._listeners )
	{
		ls->OnDamage(*this, dd);
		if( !watch )
			return;
	}

	OnDamage(world, dd);

	if( dd.damage > 0 && _health_max > 0 )
	{
		SetHealth(GetHealth() - dd.damage);
		if( GetHealth() <= 0 )
		{
			SetFlags(GC_FLAG_RBSTATIC_DESTROYED, true);
			OnDestroy(world, dd);
			Kill(world);
		}
	}
}

void GC_RigidBodyStatic::SetSize(float width, float length)
{
	_width = width;
	_length = length;
	_radius = sqrt(width*width + length*length) / 2;
}

vec2d GC_RigidBodyStatic::GetVertex(int index) const
{
	float x, y;
	switch( index )
	{
		default: assert(false);
		case 0: x =  _length / 2; y =  _width / 2; break;
		case 1: x = -_length / 2; y =  _width / 2; break;
		case 2: x = -_length / 2; y = -_width / 2; break;
		case 3: x =  _length / 2; y = -_width / 2; break;
	}
	return vec2d(GetPos().x + x * GetDirection().x - y * GetDirection().y,
				 GetPos().y + x * GetDirection().y + y * GetDirection().x);
}

void GC_RigidBodyStatic::MapExchange(MapFile &f)
{
	GC_Actor::MapExchange(f);

	MAP_EXCHANGE_FLOAT(  health,     _health,     GetDefaultHealth());
	MAP_EXCHANGE_FLOAT(  health_max, _health_max, GetDefaultHealth());
	MAP_EXCHANGE_STRING( on_destroy, _scriptOnDestroy, "");
	MAP_EXCHANGE_STRING( on_damage,  _scriptOnDamage,  "");

	if( f.loading() )
	{
		_health = std::min(_health, _health_max);
	}
}

void GC_RigidBodyStatic::Serialize(World &world, SaveFile &f)
{
	GC_Actor::Serialize(world, f);

	f.Serialize(_scriptOnDestroy);
	f.Serialize(_scriptOnDamage);
	f.Serialize(_health);
	f.Serialize(_health_max);
	f.Serialize(_radius);
	f.Serialize(_width);
	f.Serialize(_length);

	if( f.loading() && GetPassability() > 0 )
		world._field.ProcessObject(this, true);
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

	assert(false);
	return nullptr;
}

void GC_RigidBodyStatic::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_RigidBodyStatic *tmp = static_cast<GC_RigidBodyStatic *>(GetObject());

	if( applyToObject )
	{
		tmp->_scriptOnDestroy = _propOnDestroy.GetStringValue();
		tmp->_scriptOnDamage  = _propOnDamage.GetStringValue();
		tmp->SetHealth(std::min(_propMaxHealth.GetFloatValue(), _propHealth.GetFloatValue()),
                       _propMaxHealth.GetFloatValue());
	}
	else
	{
		_propHealth.SetFloatValue(tmp->GetHealth());
		_propMaxHealth.SetFloatValue(tmp->GetHealthMax());
		_propOnDestroy.SetStringValue(tmp->_scriptOnDestroy);
		_propOnDamage.SetStringValue(tmp->_scriptOnDamage);
	}
}

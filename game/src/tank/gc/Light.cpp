// light.cpp

#include "stdafx.h"

#include "light.h"

#include "level.h"
#include "macros.h"

#include "config/Config.h"

#include "video/RenderBase.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "core/debug.h"

/////////////////////////////////////////////////////////

float GC_Light::_sintable[SINTABLE_SIZE];

IMPLEMENT_SELF_REGISTRATION(GC_Light)
{
	for( int i = 0; i < SINTABLE_SIZE; ++i )
	{
		_sintable[i] = sinf((float) i / SINTABLE_SIZE * PI2);
	}
	return true;
}

GC_Light::GC_Light(enumLightType type)
  : _memberOf(this)
  , _lampSprite(new GC_2dSprite())
  , _timeout(0)
  , _aspect(1)
  , _offset(0)
  , _radius(200)
  , _type(type)
  , _lightDirection(1, 0)
  , _intensity(1)
{
	SetActive(true);
	Update();
}

GC_Light::GC_Light(FromFile)
  : GC_Actor(FromFile())
  , _memberOf(this)
{
}

GC_Light::~GC_Light()
{
}

void GC_Light::Serialize(SaveFile &f)
{
	GC_Actor::Serialize(f);

	f.Serialize(_lightDirection);
	f.Serialize(_aspect);
	f.Serialize(_intensity);
	f.Serialize(_offset);
	f.Serialize(_radius);
	f.Serialize(_timeout);
	f.Serialize(_type);
	f.Serialize(_lampSprite);
}

void GC_Light::MapExchange(MapFile &f)
{
	GC_Actor::MapExchange(f);
}

void GC_Light::Shine() const
{
	if( !IsActive() ) return;
//	_FpsCounter::Inst()->OneMoreLight();

	MyVertex *v;
	float x,y;

	SpriteColor color;
	color.dwColor = 0x00000000;
	color.a = (BYTE) __max(0, __min(255, int(255.0f * _intensity)));

	switch( _type )
	{
	case LIGHT_POINT:
		v = g_render->DrawFan(SINTABLE_SIZE>>1);
		v[0].color = color;
		v[0].x = GetPos().x;
		v[0].y = GetPos().y;
		for( int i = 0; i < SINTABLE_SIZE>>1; i++ )
		{
			v[i+1].x = GetPos().x + _radius * _sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			v[i+1].y = GetPos().y + _radius * _sintable[i<<1];
			v[i+1].color.dwColor = 0x00000000;
		}
		break;
	case LIGHT_SPOT:
		v = g_render->DrawFan(SINTABLE_SIZE);
		v[0].color = color;
		v[0].x = GetPos().x;
		v[0].y = GetPos().y;
		for( int i = 0; i < SINTABLE_SIZE; i++ )
		{
			x = _offset + _radius * _sintable[i+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			y = _radius * _sintable[i] * _aspect;
			v[i+1].x = GetPos().x + x*_lightDirection.x - y*_lightDirection.y;
			v[i+1].y = GetPos().y + y*_lightDirection.x + x*_lightDirection.y;
			v[i+1].color.dwColor = 0x00000000;
		}
		break;
	case LIGHT_DIRECT:
		v = g_render->DrawFan((SINTABLE_SIZE>>2)+4);
		v[0].color = color;
		v[0].x = GetPos().x;
		v[0].y = GetPos().y;
		for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
		{
			y = _offset * _sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			x = _offset * _sintable[i<<1];
			v[i+1].x = GetPos().x - x*_lightDirection.x - y*_lightDirection.y;
			v[i+1].y = GetPos().y - x*_lightDirection.y + y*_lightDirection.x;
			v[i+1].color.dwColor = 0x00000000;
		}

		v[(SINTABLE_SIZE>>2)+2].color.dwColor = 0x00000000;
		v[(SINTABLE_SIZE>>2)+2].x = GetPos().x + _radius * _lightDirection.x + _offset*_lightDirection.y;
		v[(SINTABLE_SIZE>>2)+2].y = GetPos().y + _radius * _lightDirection.y - _offset*_lightDirection.x;

		v[(SINTABLE_SIZE>>2)+3].color = color;
		v[(SINTABLE_SIZE>>2)+3].x = GetPos().x + _radius * _lightDirection.x;
		v[(SINTABLE_SIZE>>2)+3].y = GetPos().y + _radius * _lightDirection.y;

		v[(SINTABLE_SIZE>>2)+4].color.dwColor = 0x00000000;
		v[(SINTABLE_SIZE>>2)+4].x = GetPos().x + _radius * _lightDirection.x - _offset*_lightDirection.y;
		v[(SINTABLE_SIZE>>2)+4].y = GetPos().y + _radius * _lightDirection.y + _offset*_lightDirection.x;

		v = g_render->DrawFan((SINTABLE_SIZE>>2)+1);
		v[0].color = color;
		v[0].x = GetPos().x + _radius * _lightDirection.x;
		v[0].y = GetPos().y + _radius * _lightDirection.y;
		for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
		{
			y = _offset * _sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			x = _offset * _sintable[i<<1] + _radius;
			v[i+1].x = GetPos().x + x*_lightDirection.x - y*_lightDirection.y;
			v[i+1].y = GetPos().y + x*_lightDirection.y + y*_lightDirection.x;
			v[i+1].color.dwColor = 0x00000000;
		}
		break;
	default:
		assert(false);
	}
}

void GC_Light::Kill()
{
	SAFE_KILL(_lampSprite);
	GC_Actor::Kill();
}

void GC_Light::MoveTo(const vec2d &pos)
{
	_lampSprite->MoveTo(pos);
	GC_Actor::MoveTo(pos);
}

void GC_Light::SetTimeout(float t)
{
	assert(t > 0);
	_timeout = t;
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

void GC_Light::TimeStepFixed(float dt)
{
	assert(_timeout > 0);
	_intensity = _intensity * (_timeout - dt) / _timeout;
	_timeout -= dt;
	if( _timeout <= 0 ) Kill();
}

void GC_Light::SetActive(bool activate)
{
	SetFlags(GC_FLAG_LIGHT_ACTIVE, activate);
	_lampSprite->SetVisible(activate);
}

void GC_Light::Update()
{
	if( LIGHT_SPOT == _type )
	{
		_lampSprite->SetTexture("shine");
		_lampSprite->SetZ(g_conf.sv_nightmode.Get() ? Z_PARTICLE : Z_NONE);
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Spotlight)
{
	ED_ITEM( "spotlight", "obj_spotlight", 3 );
	return true;
}

GC_Spotlight::GC_Spotlight(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Spotlight::GC_Spotlight(float x, float y)
  : _light(new GC_Light(GC_Light::LIGHT_SPOT))
{
	_light->SetRadius(200);
	_light->SetIntensity(1.0f);
	_light->SetOffset(170);
	_light->SetAspect(0.5f);

	MoveTo(vec2d(x, y));
	SetTexture("spotlight");
	SetZ(Z_PROJECTILE);
}

GC_Spotlight::~GC_Spotlight()
{
}

void GC_Spotlight::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_light);
}

void GC_Spotlight::Kill()
{
	SAFE_KILL(_light);
	GC_2dSprite::Kill();
}

void GC_Spotlight::MoveTo(const vec2d &pos)
{
	_light->MoveTo(pos + GetDirection() * 7);
	GC_2dSprite::MoveTo(pos);
}

void GC_Spotlight::EditorAction()
{
	static vec2d delta(PI2 / 16);
	vec2d dir = Vec2dAddDirection(GetDirection(), delta);
	dir.Normalize();
	SetDirection(dir);
	_light->SetLightDirection(dir);
	_light->MoveTo(GetPos() + dir * 7);
}

void GC_Spotlight::MapExchange(MapFile &f)
{
	GC_2dSprite::MapExchange(f);

	float dir = GetDirection().Angle();
	int active = _light->IsActive();

	MAP_EXCHANGE_INT(active, active, 1);
	MAP_EXCHANGE_FLOAT(dir, dir, 0);

	if( f.loading() )
	{
		SetDirection(vec2d(dir));
		_light->SetLightDirection(GetDirection());
		_light->MoveTo(GetPos() + GetDirection() * 7);
		_light->SetActive(0 != active);
	}
}

PropertySet* GC_Spotlight::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Spotlight::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propActive( ObjectProperty::TYPE_INTEGER, "active"  )
  , _propDir(ObjectProperty::TYPE_FLOAT, "dir")
{
	_propActive.SetIntRange(0, 1);
	_propDir.SetFloatRange(0, PI2);
}

int GC_Spotlight::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_Spotlight::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propActive;
	case 1: return &_propDir;
	}

	assert(false);
	return NULL;
}

void GC_Spotlight::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Spotlight *tmp = static_cast<GC_Spotlight *>(GetObject());

	if( applyToObject )
	{	
		tmp->_light->SetActive(0 != _propActive.GetIntValue());
		tmp->SetDirection(vec2d(_propDir.GetFloatValue()));
		tmp->_light->SetLightDirection(vec2d(_propDir.GetFloatValue()));
		tmp->_light->MoveTo(tmp->GetPos() + vec2d(_propDir.GetFloatValue()) * 7);
	}
	else
	{
		_propActive.SetIntValue(tmp->_light->IsActive() ? 1 : 0);
		_propDir.SetFloatValue(tmp->GetDirection().Angle());
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

// light.cpp

#include "stdafx.h"
#include "light.h"
#include "options.h"
#include "level.h"
#include "macros.h"

#include "config/Config.h"

#include "video/RenderBase.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "gc/GameClasses.h"
#include "gc/Editor.h"
#include "gc/RigidBody.h"



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
  : _memberOf(g_level->lights, this)
{
	_timeout    = 0;
	_aspect     = 1;
	_offset     = 0;
	_radius     = 200;
	_type       = type;
	_angle      = 0;
	_intensity  = 1;
	SetFlags(GC_FLAG_LIGHT_ENABLED);

	_lamp = new GC_UserSprite();

	if( g_conf.sv_nightmode->Get() ) // FIXME!
	if( LIGHT_SPOT == type )
	{
		_lamp->SetTexture("shine");
		_lamp->SetZ(Z_PARTICLE);
	}
}

GC_Light::GC_Light(FromFile)
    : GC_Object(FromFile()), _memberOf(g_level->lights, this)
{
}

GC_Light::~GC_Light()
{
}

void GC_Light::Serialize(SaveFile &f)
{
	GC_Object::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_angle);
	f.Serialize(_aspect);
	f.Serialize(_intensity);
	f.Serialize(_offset);
	f.Serialize(_radius);
	f.Serialize(_timeout);
	f.Serialize(_type);
	/////////////////////////////////////
	f.Serialize(_lamp);
}

void GC_Light::Shine()
{
	if( !CheckFlags(GC_FLAG_LIGHT_ENABLED) ) return;
	_FpsCounter::Inst()->OneMoreLight();

	MyVertex *v;
	float s,c, x,y;

	SpriteColor color;
	color.dwColor = 0x00000000;
	color.a = (BYTE) __max(0, __min(255, int(255.0f * _intensity)));

	switch( _type )
	{
	case LIGHT_POINT:
		v = g_render->DrawFan(SINTABLE_SIZE>>1);
		v[0].color = color;
		v[0].x = _pos.x;
		v[0].y = _pos.y;
		for( int i = 0; i < SINTABLE_SIZE>>1; i++ )
		{
			v[i+1].x = _pos.x + _radius * _sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			v[i+1].y = _pos.y + _radius * _sintable[i<<1];
			v[i+1].color.dwColor = 0x00000000;
		}
		break;
	case LIGHT_SPOT:
		s = sinf(_angle); c = cosf(_angle);
		v = g_render->DrawFan(SINTABLE_SIZE);
		v[0].color = color;
		v[0].x = _pos.x;
		v[0].y = _pos.y;
		for( int i = 0; i < SINTABLE_SIZE; i++ )
		{
			x = _offset + _radius * _sintable[i+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			y = _radius * _sintable[i] * _aspect;
			v[i+1].x = _pos.x + x*c - y*s;
			v[i+1].y = _pos.y + y*c + x*s;
			v[i+1].color.dwColor = 0x00000000;
		}
		break;
	case LIGHT_DIRECT:
		s = sinf(_angle); c = cosf(_angle);
		v = g_render->DrawFan((SINTABLE_SIZE>>2)+4);
		v[0].color = color;
		v[0].x = _pos.x;
		v[0].y = _pos.y;
		for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
		{
			y = _offset * _sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			x = _offset * _sintable[i<<1];
			v[i+1].x = _pos.x - x*c - y*s;
			v[i+1].y = _pos.y - x*s + y*c;
			v[i+1].color.dwColor = 0x00000000;
		}

		v[(SINTABLE_SIZE>>2)+2].color.dwColor = 0x00000000;
		v[(SINTABLE_SIZE>>2)+2].x = _pos.x + _radius * c + _offset*s;
		v[(SINTABLE_SIZE>>2)+2].y = _pos.y + _radius * s - _offset*c;

		v[(SINTABLE_SIZE>>2)+3].color = color;
		v[(SINTABLE_SIZE>>2)+3].x = _pos.x + _radius * c;
		v[(SINTABLE_SIZE>>2)+3].y = _pos.y + _radius * s;

		v[(SINTABLE_SIZE>>2)+4].color.dwColor = 0x00000000;
		v[(SINTABLE_SIZE>>2)+4].x = _pos.x + _radius * c - _offset*s;
		v[(SINTABLE_SIZE>>2)+4].y = _pos.y + _radius * s + _offset*c;

		v = g_render->DrawFan((SINTABLE_SIZE>>2)+1);
		v[0].color = color;
		v[0].x = _pos.x + _radius * c;
		v[0].y = _pos.y + _radius * s;
		for( int i = 0; i <= SINTABLE_SIZE>>2; i++ )
		{
			y = _offset * _sintable[(i<<1)+(SINTABLE_SIZE>>2) & SINTABLE_MASK];
			x = _offset * _sintable[i<<1] + _radius;
			v[i+1].x = _pos.x + x*c - y*s;
			v[i+1].y = _pos.y + x*s + y*c;
			v[i+1].color.dwColor = 0x00000000;
		}
		break;
	default:
		_ASSERT(FALSE);
	}
}

void GC_Light::Enable(bool bEnable)
{
	bEnable?SetFlags(GC_FLAG_LIGHT_ENABLED):ClearFlags(GC_FLAG_LIGHT_ENABLED);
	_lamp->Show(bEnable);
}

void GC_Light::Kill()
{
	SAFE_KILL(_lamp);
	GC_Object::Kill();
}

void GC_Light::MoveTo(const vec2d &pos)
{
	_lamp->MoveTo(pos);
	GC_Object::MoveTo(pos);
}

void GC_Light::SetTimeout(float t)
{
	_ASSERT(t > 0);
	_timeout = t;
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

void GC_Light::TimeStepFixed(float dt)
{
	_ASSERT(_timeout > 0);
	_intensity = _intensity * (_timeout - dt) / _timeout;
	_timeout -= dt;
	if( _timeout <= 0 ) Kill();
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Spotlight)
{
	ED_ITEM( "spotlight", "Объект:  Прожектор             " );
	return true;
}

GC_Spotlight::GC_Spotlight(FromFile) : GC_2dSprite(FromFile())
{
}

GC_Spotlight::GC_Spotlight(float x, float y)
{
	_light = new GC_Light(GC_Light::LIGHT_SPOT);
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
	//------------------------------
	f.Serialize(_light);
}

void GC_Spotlight::Kill()
{
	SAFE_KILL(_light);
	GC_2dSprite::Kill();
}

void GC_Spotlight::MoveTo(const vec2d &pos)
{
	_light->MoveTo(pos+vec2d(GetRotation())*7);
	GC_2dSprite::MoveTo(pos);
}

void GC_Spotlight::EditorAction()
{
	float a = GetRotation();
	a += PI2 / 16;
	a = fmodf(a, PI2);

	SetRotation(a);
	_light->SetAngle(a);
	_light->MoveTo(_pos+vec2d(a)*7);
}

void GC_Spotlight::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);
	//-----------------------------------------
	float a = GetRotation();
	MAP_EXCHANGE_FLOAT(dir, a, 0);

	if( f.loading() )
	{
		SetRotation(a);
		_light->SetAngle(a);
		_light->MoveTo(_pos+vec2d(a)*7);
	}
}

IPropertySet* GC_Spotlight::GetProperties()
{
	return NULL;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

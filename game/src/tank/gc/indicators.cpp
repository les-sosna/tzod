// indicators.cpp

#include "stdafx.h"
#include "indicators.h"
#include "Vehicle.h"

#include "level.h"
#include "macros.h"
#include "functions.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "core/debug.h"


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_SpawnPoint)
{
	ED_ACTOR("respawn_point", "obj_respawn",
	    0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, CELL_SIZE/2);
	return true;
}

GC_SpawnPoint::GC_SpawnPoint(float x, float y)
  : GC_2dSprite()
  , _memberOf(this)
{
	SetTexture("editor_respawn");
	SetZ(Z_EDITOR);
	MoveTo(vec2d(x, y));
	_team = 0;
}

GC_SpawnPoint::GC_SpawnPoint(FromFile)
  : GC_2dSprite(FromFile())
  , _memberOf(this)
{
}

void GC_SpawnPoint::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_team);
}

void GC_SpawnPoint::Draw() const
{
	if( g_level->_modeEditor )
	{
		__super::Draw();

		static const char* teams[MAX_TEAMS] = {"", "1", "2", "3", "4", "5"};
		_ASSERT(_team >= 0 && _team < MAX_TEAMS);
		g_level->DrawText(teams[_team], GetPos(), alignTextCC);
	}
}

void GC_SpawnPoint::EditorAction()
{
	float rotation = GetSpriteRotation();
	rotation += PI/3;
	rotation -= fmodf(rotation, PI/4);
	SetSpriteRotation(rotation);
}

void GC_SpawnPoint::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);

	float dir = GetSpriteRotation();

	MAP_EXCHANGE_FLOAT(dir, dir, 0);
	MAP_EXCHANGE_INT(team, _team, 0);

	SetSpriteRotation(dir);

	if( _team > MAX_TEAMS-1 )
		_team = MAX_TEAMS-1;
}


PropertySet* GC_SpawnPoint::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_SpawnPoint::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTeam( ObjectProperty::TYPE_INTEGER, "team" )
  , _propDir( ObjectProperty::TYPE_FLOAT, "dir" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS - 1);
	_propDir.SetFloatRange(0, PI2);
}

int GC_SpawnPoint::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_SpawnPoint::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTeam;
	case 1: return &_propDir;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_SpawnPoint::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_SpawnPoint *tmp = static_cast<GC_SpawnPoint *>(GetObject());

	if( applyToObject )
	{
		tmp->_team = _propTeam.GetIntValue();
		tmp->SetSpriteRotation(_propDir.GetFloatValue());
	}
	else
	{
		_propTeam.SetIntValue(tmp->_team);
		_propDir.SetFloatValue(fmodf(tmp->GetSpriteRotation(), PI2));
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HideLabel)
{
	return true;
}

GC_HideLabel::GC_HideLabel(float x, float y)
  : GC_2dSprite()
{
	SetZ(Z_EDITOR);
	SetTexture("editor_item");
	MoveTo(vec2d(x, y));
}

GC_HideLabel::GC_HideLabel(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_HideLabel::Draw() const
{
	if( g_level->_modeEditor )
	{
		__super::Draw();
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Crosshair)
{
	return true;
}

GC_Crosshair::GC_Crosshair(enChStyle style)
  : GC_2dSprite()
{
	SetZ(Z_VEHICLE_LABEL);

	_time = 0;
	_chStyle = style;

	switch (_chStyle)
	{
	case CHS_SINGLE:
		SetTexture("indicator_crosshair1");
		break;
	case CHS_DOUBLE:
		SetTexture("indicator_crosshair2");
		break;
	default:
		_ASSERT(FALSE);
	}

	///////////////////////
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
}

GC_Crosshair::GC_Crosshair(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_Crosshair::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_angle);
	f.Serialize(_chStyle);
	f.Serialize(_time);
}

void GC_Crosshair::TimeStepFloat(float dt)
{
	GC_2dSprite::TimeStepFloat(dt);

	_time += dt;

	switch (_chStyle)
	{
	case CHS_SINGLE:
		SetSpriteRotation(_time*5);
		break;
	case CHS_DOUBLE:
		SetSpriteRotation(_angle);
		break;
	default:
		_ASSERT(FALSE);
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_IndicatorBar)
{
	return true;
}

GC_IndicatorBar::GC_IndicatorBar(const char *texture, GC_2dSprite *object,
                                 float *pValue, float *pValueMax, LOCATION location)
  : GC_2dSprite()
  , _memberOf(this)
{
	_ASSERT(NULL == FindIndicator(object, location));

	SetZ(Z_VEHICLE_LABEL);

	SetTexture(texture);

	_dwValueMax_offset = (DWORD) pValueMax - (DWORD) object;
	_dwValue_offset    = (DWORD) pValue    - (DWORD) object;

	_location = location;

	_object = WrapRawPtr(object);

	_object->Subscribe(NOTIFY_OBJECT_KILL, this,
		(NOTIFYPROC) &GC_IndicatorBar::OnParentKill, true, true);

	GC_2dSprite *sprite = object;
	if( GC_Vehicle *veh = dynamic_cast<GC_Vehicle *>(object) )
	{
		sprite = veh->GetVisual();
	}
	sprite->Subscribe(NOTIFY_ACTOR_MOVE, this,
		(NOTIFYPROC) &GC_IndicatorBar::OnUpdatePosition, false, true);
	OnUpdatePosition(sprite, NULL);
}

GC_IndicatorBar::GC_IndicatorBar(FromFile)
  : GC_2dSprite(FromFile())
  , _memberOf(this)
{
}

void GC_IndicatorBar::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_dwValueMax_offset);
	f.Serialize(_dwValue_offset);
	f.Serialize(_location);
	f.Serialize(_object);
}

void GC_IndicatorBar::Kill()
{
	_object = NULL;
	GC_2dSprite::Kill();
}

GC_IndicatorBar* GC_IndicatorBar::FindIndicator(GC_2dSprite* pFind, LOCATION location)
{
	FOREACH( g_level->GetList(LIST_indicators), GC_IndicatorBar, object )
	{
		if( pFind == object->_object && location == object->_location )
			return object;
	}

	return NULL;
}

void GC_IndicatorBar::Draw() const
{
	_ASSERT(_object);

	float val     = *((float *) ((char*) GetRawPtr(_object) + _dwValue_offset));
	float max_val = *((float *) ((char*) GetRawPtr(_object) + _dwValueMax_offset));

	if( max_val > 0 )
	{
		if( val < 0 )        val = 0;
		if( val > max_val )  val = max_val;
		if( CheckFlags(GC_FLAG_INDICATOR_INVERSE) )
			val = max_val - val;

		vec2d pos = GetPosPredicted();
		g_texman->DrawIndicator(GetTexture(), pos.x, pos.y, val / max_val);
	}
}


void GC_IndicatorBar::OnUpdatePosition(GC_Object *sender, void *param)
{
	ASSERT_TYPE(sender, GC_2dSprite);
	GC_2dSprite *sprite = static_cast<GC_2dSprite *>(sender);

	vec2d pos = sprite->GetPosPredicted();
	FRECT rt;
	sprite->GetLocalRect(rt);
	float top = pos.y + rt.top;

	switch( _location )
	{
	case LOCATION_TOP:
		MoveTo( vec2d(pos.x, __max(top - GetSpriteHeight(), 0)) );
		break;
	case LOCATION_BOTTOM:
		MoveTo( vec2d(pos.x, __min(top + sprite->GetSpriteHeight() + GetSpriteHeight(),
			g_level->_sy - GetSpriteHeight()*2)) );
		break;
	default:
		_ASSERT(FALSE);
	}
}

void GC_IndicatorBar::OnParentKill(GC_Object *sender, void *param)
{
	Kill();
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_DamLabel)
{
	return true;
}

GC_DamLabel::GC_DamLabel(GC_VehicleVisualDummy *veh)
  : GC_2dSprite()
{
	SetTexture("indicator_damage");
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
	SetZ(Z_VEHICLE_LABEL);

	veh->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_DamLabel::OnVehicleMove, false);

	_time = 0;
	_time_life = 0.4f;
	_rot = frand(PI2);
}

GC_DamLabel::GC_DamLabel(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_DamLabel::~GC_DamLabel()
{
}

void GC_DamLabel::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_rot);
	f.Serialize(_time);
	f.Serialize(_time_life);
}

void GC_DamLabel::TimeStepFloat(float dt)
{
	GC_2dSprite::TimeStepFloat(dt);

	_time += dt;
	SetSpriteRotation(_rot + _time * 2.0f);

	if( _time >= _time_life )
	{
		Kill();
	}
	else
	{
		SetFrame(int(_time * 10.0f) % GetFrameCount());
	}
}

void GC_DamLabel::Reset()
{
	_time_life = _time + 0.4f;
}

void GC_DamLabel::OnVehicleMove(GC_Object *sender, void *param)
{
	MoveTo(static_cast<GC_Actor*>(sender)->GetPos());
}

// end of file

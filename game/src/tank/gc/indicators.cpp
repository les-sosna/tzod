// indicators.cpp

#include "stdafx.h"
#include "indicators.h"

#include "level.h"
#include "macros.h"

#include "fs/MapFile.h"
#include "fs/SaveFile.h"

#include "Vehicle.h"

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_SpawnPoint)
{
	ED_ACTOR("respawn_point", "Точка рождения танка", 
	    0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, CELL_SIZE/2);
	return true;
}

GC_SpawnPoint::GC_SpawnPoint(float x, float y)
  : GC_2dSprite(), _memberOf(g_level->respawns, this)
{
	SetZ(Z_EDITOR);

	SetTexture("editor_respawn");

	MoveTo(vec2d(x, y));
	_team = 0;
}

GC_SpawnPoint::GC_SpawnPoint(FromFile)
  : GC_2dSprite(FromFile()), _memberOf(g_level->respawns, this)
{
}

void GC_SpawnPoint::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	f.Serialize(_team);
}

void GC_SpawnPoint::Draw()
{
	if( g_level->_modeEditor )
	{
		GC_2dSprite::Draw();
		static const char* teams[MAX_TEAMS] = {"", "1", "2", "3", "4", "5"};
		_ASSERT(_team >= 0 && _team < MAX_TEAMS);
		g_level->DrawText(teams[_team], GetPos(), alignTextCC);
	}
}

void GC_SpawnPoint::EditorAction()
{
	float rotation = GetRotation();
	rotation += PI/3;
	rotation -= fmodf(rotation, PI/4);
	SetRotation(rotation);
}

void GC_SpawnPoint::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);

	float dir = GetRotation();

	MAP_EXCHANGE_FLOAT(dir, dir, 0);
	MAP_EXCHANGE_INT(team, _team, 0);

	SetRotation(dir);

	if( _team > MAX_TEAMS-1 )
		_team = MAX_TEAMS-1;
}


SafePtr<PropertySet> GC_SpawnPoint::GetProperties()
{
	return new MyPropertySet(this);
}

GC_SpawnPoint::MyPropertySet::MyPropertySet(GC_Object *object)
: BASE(object)
, _propTeam( ObjectProperty::TYPE_INTEGER, "team" )
{
	_propTeam.SetRange(0, MAX_TEAMS);
	Exchange(false);
}

int GC_SpawnPoint::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_SpawnPoint::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTeam;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_SpawnPoint::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

	GC_SpawnPoint *tmp = static_cast<GC_SpawnPoint *>(GetObject());

	if( applyToObject )
	{
		tmp->_team = _propTeam.GetValueInt();
	}
	else
	{
		_propTeam.SetValueInt(tmp->_team);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HideLabel)
{
	return true;
}

GC_HideLabel::GC_HideLabel(float x, float y) : GC_2dSprite()
{
	SetZ(Z_EDITOR);
	SetTexture("editor_item");
	MoveTo(vec2d(x, y));
}

GC_HideLabel::GC_HideLabel(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_HideLabel::Draw()
{
	if( g_level->_modeEditor )
		GC_2dSprite::Draw();
}

//////////////////////////////////////////////////////////////////////////////////////////////


/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Crosshair)
{
	return true;
}

GC_Crosshair::GC_Crosshair(enChStyle style) : GC_2dSprite()
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

GC_Crosshair::GC_Crosshair(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_Crosshair::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_2dSprite::Serialize(f);
	/////////////////////////////////////
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
		SetRotation(_time*5);
		break;
	case CHS_DOUBLE:
		SetRotation(_angle);
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

GC_IndicatorBar::GC_IndicatorBar(const char *texture, GC_2dSprite* object,
								 float *pValue, float *pValueMax, LOCATION location)
  : GC_2dSprite(), _memberOf(g_level->indicators, this)
{
	_ASSERT(NULL == FindIndicator(object, location));

	SetZ(Z_VEHICLE_LABEL);

	SetTexture(texture);
	_initial_width = GetSpriteWidth();

	_dwValueMax_offset = (DWORD) pValueMax - (DWORD) object;
	_dwValue_offset    = (DWORD) pValue    - (DWORD) object;

	_location = location;
	SetInverse(false);

	_object = object;
	///////////////////////
	_object->Subscribe(NOTIFY_OBJECT_KILL, this,
		(NOTIFYPROC) &GC_IndicatorBar::OnParentKill, true, true);
	_object->Subscribe(NOTIFY_ACTOR_MOVE, this,
		(NOTIFYPROC) &GC_IndicatorBar::OnUpdate, false, true);
	_object->Subscribe(NOTIFY_OBJECT_UPDATE_INDICATOR, this,
		(NOTIFYPROC) &GC_IndicatorBar::OnUpdate, false, true);
	//
//	OnUpdate(this, object, NULL);
}

GC_IndicatorBar::GC_IndicatorBar(FromFile)
  : GC_2dSprite(FromFile()), _memberOf(g_level->indicators, this)
{
}

void GC_IndicatorBar::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_2dSprite::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_bInverse);
	f.Serialize(_dwValueMax_offset);
	f.Serialize(_dwValue_offset);
	f.Serialize(_initial_width);
	f.Serialize(_location);
	/////////////////////////////////////
	f.Serialize(_object);
}

void GC_IndicatorBar::Kill()
{
	_object = NULL;
	GC_2dSprite::Kill();
}

GC_IndicatorBar* GC_IndicatorBar::FindIndicator(GC_2dSprite* pFind, LOCATION location)
{
	FOREACH( indicators, GC_IndicatorBar, object )
	{
		if( pFind == object->_object && location == object->_location )
			return object;
	}

	return NULL;
}

void GC_IndicatorBar::OnUpdate(GC_Object *sender, void *param)
{
	_ASSERT(sender == _object);
	_ASSERT(_object != NULL);
	_ASSERT(!_object->IsKilled());

	float val     = *((float *) ((char*) GetRawPtr(_object) + _dwValue_offset));
	float max_val = *((float *) ((char*) GetRawPtr(_object) + _dwValueMax_offset));

	if( max_val > 0 )
	{
		//
		// update value
		//

		if( val < 0 )        val = 0;
		if( val > max_val )  val = max_val;
		if( _bInverse )      val = max_val - val;

		SetFrame(0);
		FRECT rt;
		rt.left = 0;
		rt.top  = 0;
		rt.bottom = 1;
		rt.right = val / max_val;
		ModifyFrameBounds(&rt);
		Resize(_initial_width * rt.right, GetSpriteHeight());


		//
		// update position
		//

		const GC_2dSprite *sprite = static_cast<GC_2dSprite*>(sender);

		FRECT frame;
		sprite->GetGlobalRect(frame);

		switch( _location )
		{
		case LOCATION_TOP:
			MoveTo( vec2d(sprite->GetPos().x - _initial_width / 2.0f,
				__max(frame.top - GetSpriteHeight(), 0)) );
			break;
		case LOCATION_BOTTOM:
			MoveTo( vec2d(sprite->GetPos().x - _initial_width / 2.0f,
				__min(frame.bottom + GetSpriteHeight(),
				g_level->_sy - GetSpriteHeight()*2)) );
			break;
		default:
			_ASSERT(FALSE);
		}

		Show(true);
	}
	else
	{
		Show(false);
	}
}

void GC_IndicatorBar::OnParentKill(GC_Object *sender, void *param)
{
	Kill();
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_DamLabel)
{
	return true;
}

GC_DamLabel::GC_DamLabel(GC_Vehicle *veh) : GC_2dSprite()
{
	SetZ(Z_VEHICLE_LABEL);

	_vehicle = veh;
	_vehicle->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_DamLabel::OnVehicleMove, false);

	_time = 0;
	_time_life = 0.4f;

	SetTexture("indicator_damage");
	///////////////////////
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING);
}

GC_DamLabel::GC_DamLabel(FromFile) : GC_2dSprite(FromFile())
{
}

GC_DamLabel::~GC_DamLabel()
{
}

void GC_DamLabel::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_2dSprite::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_time);
	f.Serialize(_time_life);
	/////////////////////////////////////
	f.Serialize(_vehicle);
}

void GC_DamLabel::Kill()
{
	_vehicle = NULL;
	GC_2dSprite::Kill();
}

void GC_DamLabel::TimeStepFloat(float dt)
{
	GC_2dSprite::TimeStepFloat(dt);

	_ASSERT(_vehicle && !_vehicle->IsKilled());

	_time += dt;
	SetRotation(_time*2.0f);

	if( _time >= _time_life )
	{
		_vehicle->_damLabel = NULL;
		Kill();
	}
	else
	{
		SetFrame( int(_time * 10.0f)%GetFrameCount() );
	}
}

void GC_DamLabel::Draw()
{
	GC_2dSprite::Draw();
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

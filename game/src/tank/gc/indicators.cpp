// indicators.cpp

#include "indicators.h"

#include "GlobalListHelper.inl"
#include "World.h"
#include "Macros.h"
#include "MapFile.h"
#include "SaveFile.h"
#include "Vehicle.h"

#include "core/debug.h"


///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_SpawnPoint)
{
	ED_ACTOR("respawn_point", "obj_respawn",
	    0, CELL_SIZE, CELL_SIZE, CELL_SIZE/2, CELL_SIZE/2);
	return true;
}

GC_SpawnPoint::GC_SpawnPoint(World &world, float x, float y)
  : GC_2dSprite(world)
  , _memberOf(this)
{
	SetTexture("editor_respawn");
	SetZ(world, Z_EDITOR);
	MoveTo(world, vec2d(x, y));
	_team = 0;
}

GC_SpawnPoint::GC_SpawnPoint(FromFile)
  : GC_2dSprite(FromFile())
  , _memberOf(this)
{
}

void GC_SpawnPoint::Serialize(World &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);
	f.Serialize(_team);
}

void GC_SpawnPoint::Draw(bool editorMode) const
{
	if( editorMode )
	{
		GC_2dSprite::Draw(editorMode);

		static const char* teams[MAX_TEAMS] = {"", "1", "2", "3", "4", "5"};
		assert(_team >= 0 && _team < MAX_TEAMS);
		static size_t font = g_texman->FindSprite("font_default");
		g_texman->DrawBitmapText(GetPos().x, GetPos().y, font, 0xffffffff, teams[_team], alignTextCC);
	}
}

void GC_SpawnPoint::EditorAction(World &world)
{
	float rotation = GetDirection().Angle();
	rotation += PI/3;
	rotation -= fmod(rotation, PI/4);
	SetDirection(vec2d(rotation));
}

void GC_SpawnPoint::MapExchange(World &world, MapFile &f)
{
	GC_2dSprite::MapExchange(world, f);

	float dir = GetDirection().Angle();

	MAP_EXCHANGE_FLOAT(dir, dir, 0);
	MAP_EXCHANGE_INT(team, _team, 0);

	if( f.loading() )
	{
		SetDirection(vec2d(dir));
		if( _team > MAX_TEAMS-1 )
			_team = MAX_TEAMS-1;
	}
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

	assert(false);
	return NULL;
}

void GC_SpawnPoint::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_SpawnPoint *tmp = static_cast<GC_SpawnPoint *>(GetObject());

	if( applyToObject )
	{
		tmp->_team = _propTeam.GetIntValue();
		tmp->SetDirection(vec2d(_propDir.GetFloatValue()));
	}
	else
	{
		_propTeam.SetIntValue(tmp->_team);
		_propDir.SetFloatValue(tmp->GetDirection().Angle());
	}
}

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_HideLabel)
{
	return true;
}

GC_HideLabel::GC_HideLabel(World &world, float x, float y)
  : GC_2dSprite(world)
{
	SetZ(world, Z_EDITOR);
	SetTexture("editor_item");
	MoveTo(world, vec2d(x, y));
}

GC_HideLabel::GC_HideLabel(FromFile)
  : GC_2dSprite(FromFile())
{
}

void GC_HideLabel::Draw(bool editorMode) const
{
	if( editorMode )
	{
		GC_2dSprite::Draw(editorMode);
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_IndicatorBar)
{
	return true;
}

GC_IndicatorBar::GC_IndicatorBar(World &world, const char *texture, GC_2dSprite *object,
                                 float *pValue, float *pValueMax, LOCATION location)
  : GC_2dSprite(world)
  , _memberOf(this)
{
	assert(NULL == FindIndicator(world, object, location));

	SetZ(world, Z_VEHICLE_LABEL);

	SetTexture(texture);

	_dwValueMax_offset = (size_t) pValueMax - (size_t) object;
	_dwValue_offset    = (size_t) pValue    - (size_t) object;

	_location = location;

	_object = object;
	_object->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_IndicatorBar::OnParentKill);
	_object->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_IndicatorBar::OnUpdatePosition);
	OnUpdatePosition(world, object, NULL);
}

GC_IndicatorBar::GC_IndicatorBar(FromFile)
  : GC_2dSprite(FromFile())
  , _memberOf(this)
{
}

void GC_IndicatorBar::Serialize(World &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);

	f.Serialize(_dwValueMax_offset);
	f.Serialize(_dwValue_offset);
	f.Serialize(_location);
	f.Serialize(_object);
}

GC_IndicatorBar* GC_IndicatorBar::FindIndicator(World &world, GC_2dSprite* pFind, LOCATION location)
{
	FOREACH( world.GetList(LIST_indicators), GC_IndicatorBar, object )
	{
		if( pFind == object->_object && location == object->_location )
			return object;
	}

	return NULL;
}

void GC_IndicatorBar::Draw(bool editorMode) const
{
	assert(_object);

	float val     = *((float *) ((char*) (GC_Object *) _object + _dwValue_offset));
	float max_val = *((float *) ((char*) (GC_Object *) _object + _dwValueMax_offset));

	if( max_val > 0 )
	{
		if( val < 0 )        val = 0;
		if( val > max_val )  val = max_val;
		if( CheckFlags(GC_FLAG_INDICATOR_INVERSE) )
			val = max_val - val;

		vec2d pos = GetPos();
		g_texman->DrawIndicator(GetTexture(), pos.x, pos.y, val / max_val);
	}
}


void GC_IndicatorBar::OnUpdatePosition(World &world, GC_Object *sender, void *param)
{
	ASSERT_TYPE(sender, GC_2dSprite);
	GC_2dSprite *sprite = static_cast<GC_2dSprite *>(sender);

	vec2d pos = sprite->GetPos();
	FRECT rt;
	sprite->GetLocalRect(rt);
	float top = pos.y + rt.top;

	switch( _location )
	{
	case LOCATION_TOP:
        MoveTo(world, vec2d(pos.x, std::max(top - GetSpriteHeight(), .0f)));
		break;
	case LOCATION_BOTTOM:
        MoveTo(world, vec2d(pos.x, std::min(top + sprite->GetSpriteHeight() + GetSpriteHeight(), world._sy - GetSpriteHeight()*2)));
		break;
	default:
		assert(false);
	}
}

void GC_IndicatorBar::OnParentKill(World &world, GC_Object *sender, void *param)
{
	Kill(world);
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_DamLabel)
{
	return true;
}

GC_DamLabel::GC_DamLabel(World &world, GC_Vehicle *veh)
  : GC_2dSprite(world)
  , _phase(frand(PI2))
  , _time(0)
  , _time_life(0.4f)
{
	SetTexture("indicator_damage");
	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetZ(world, Z_VEHICLE_LABEL);
	veh->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_DamLabel::OnVehicleMove);
}

GC_DamLabel::GC_DamLabel(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_DamLabel::~GC_DamLabel()
{
}

void GC_DamLabel::Serialize(World &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);

	f.Serialize(_phase);
	f.Serialize(_time);
	f.Serialize(_time_life);
}

void GC_DamLabel::TimeStepFloat(World &world, float dt)
{
	GC_2dSprite::TimeStepFloat(world, dt);

	_time += dt;
	SetDirection(vec2d(_phase + _time * 2.0f));

	if( _time >= _time_life )
	{
		Kill(world);
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

void GC_DamLabel::OnVehicleMove(World &world, GC_Object *sender, void *param)
{
	MoveTo(world, static_cast<GC_Actor*>(sender)->GetPos());
}

// end of file

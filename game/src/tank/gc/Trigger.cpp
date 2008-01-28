// Trigger.cpp

#include "stdafx.h"
#include "Trigger.h"
#include "Vehicle.h"
#include "ai.h"

#include "level.h"
#include "Macros.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "core/Console.h"

IMPLEMENT_SELF_REGISTRATION(GC_Trigger)
{
	ED_ITEM( "trigger", "Триггер", 6 );
	return true;
}


GC_Trigger::GC_Trigger(float x, float y) : GC_2dSprite()
{
	SetTexture("editor_trigger");
	MoveTo(vec2d(x, y));
	SetZ(Z_WOOD);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetFlags(GC_FLAG_TRIGGER_ACTIVE);
	SetFlags(GC_FLAG_TRIGGER_ONLYVISIBLE);

	_team = 0;
	_radius = 1;
	_radiusDelta = 0;
}

GC_Trigger::GC_Trigger(FromFile) : GC_2dSprite(FromFile())
{
}

GC_Trigger::~GC_Trigger()
{
}

bool GC_Trigger::IsVisible(const GC_Vehicle *v)
{
	GC_RigidBodyStatic *object = g_level->agTrace(
		g_level->grid_rigid_s, NULL, GetPos(), v->GetPos() - GetPos());
	return object == v;
}

void GC_Trigger::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);

	f.Serialize(_radius);
	f.Serialize(_radiusDelta);
	f.Serialize(_onEnter);
	f.Serialize(_onLeave);
	f.Serialize(_veh);
	f.Serialize(_team);
}

void GC_Trigger::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);

	int onlyVisible = CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE);
	int onlyHuman = CheckFlags(GC_FLAG_TRIGGER_ONLYHUMAN);
	int active = CheckFlags(GC_FLAG_TRIGGER_ACTIVE);

	MAP_EXCHANGE_INT(active, active, 1);
	MAP_EXCHANGE_INT(only_visible, onlyVisible, 0);
	MAP_EXCHANGE_INT(only_human, onlyHuman, 0);
	MAP_EXCHANGE_INT(team, _team, 0);
	MAP_EXCHANGE_FLOAT(radius, _radius, 1);
	MAP_EXCHANGE_FLOAT(radius_delta, _radiusDelta, 0);
	MAP_EXCHANGE_STRING(on_enter, _onEnter, "");
	MAP_EXCHANGE_STRING(on_leave, _onLeave, "");

	if( f.loading() )
	{
		onlyVisible ? SetFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) : ClearFlags(GC_FLAG_TRIGGER_ONLYVISIBLE);
		onlyHuman ? SetFlags(GC_FLAG_TRIGGER_ONLYHUMAN) : ClearFlags(GC_FLAG_TRIGGER_ONLYHUMAN);
		active ? SetFlags(GC_FLAG_TRIGGER_ACTIVE) : ClearFlags(GC_FLAG_TRIGGER_ACTIVE);
	}
}

void GC_Trigger::TimeStepFixed(float dt)
{
	if( !_veh && CheckFlags(GC_FLAG_TRIGGER_ACTIVE) )
	{
		// find nearest vehicle
		float rr_min = _radius * _radius * CELL_SIZE * CELL_SIZE;
		FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, veh )
		{
			if( !veh->IsKilled() )
			{
				if( NULL == veh->GetPlayer()
					|| CheckFlags(GC_FLAG_TRIGGER_ONLYHUMAN) 
					&& dynamic_cast<GC_PlayerAI*>(veh->GetPlayer()) )
				{
					continue;
				}
				if( _team && veh->GetPlayer()->GetTeam() != _team )
				{
					continue;
				}
				float rr = (GetPos() - veh->GetPos()).sqr();
				if( rr < rr_min )
				{
					if( CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) && rr > veh->_radius * veh->_radius )
					{
						if( !IsVisible(veh) ) continue; // vehicle is invisible. skipping
					}
					rr_min = rr;
					_veh = veh;
				}
			}
		}
		if( _veh )
		{
			const char *who = _veh->GetName();
			std::stringstream buf;
			buf << "return function(who)";
			buf << _onEnter;
			buf << "\nend";

			if( luaL_loadstring(g_env.L, buf.str().c_str()) )
			{
				g_console->printf("syntax error %s\n", lua_tostring(g_env.L, -1));
				lua_pop(g_env.L, 1); // pop the error message from the stack
			}
			else
			{
				if( lua_pcall(g_env.L, 0, 1, 0) )
				{
					g_console->printf("%s\n", lua_tostring(g_env.L, -1));
					lua_pop(g_env.L, 1); // pop the error message from the stack
				}
				lua_pushstring(g_env.L, who ? who : "");
				if( lua_pcall(g_env.L, 1, 0, 0) )
				{
					g_console->printf("%s\n", lua_tostring(g_env.L, -1));
					lua_pop(g_env.L, 1); // pop the error message from the stack
				}
			}
		}
	}
	else if( _veh )
	{
		if( _veh->IsKilled() )
		{
			script_exec(g_env.L, _onLeave.c_str());
			_veh = NULL;
			return;
		}

		float rr = (GetPos() - _veh->GetPos()).sqr();
		float r = (_radius + _radiusDelta) * CELL_SIZE;
		if( rr > r*r || CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) 
			&& rr > _veh->_radius * _veh->_radius && !IsVisible(GetRawPtr(_veh)) )
		{
			script_exec(g_env.L, _onLeave.c_str());
			_veh = NULL;
			return;
		}
	}
}

void GC_Trigger::Draw()
{
	if( g_level->_modeEditor )
		GC_2dSprite::Draw();
}

PropertySet* GC_Trigger::NewPropertySet()
{
	return new MyPropertySet(this);
}


GC_Trigger::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propActive(ObjectProperty::TYPE_INTEGER, "active")
  , _propTeam(ObjectProperty::TYPE_INTEGER, "team")
  , _propRadius(ObjectProperty::TYPE_FLOAT, "radius")
  , _propRadiusDelta(ObjectProperty::TYPE_FLOAT, "radius_delta")
  , _propOnlyHuman(ObjectProperty::TYPE_INTEGER, "only_human")
  , _propOnlyVisible(ObjectProperty::TYPE_INTEGER, "only_visible")
  , _propOnEnter(ObjectProperty::TYPE_STRING, "on_enter")
  , _propOnLeave(ObjectProperty::TYPE_STRING, "on_leave")
{
	_propActive.SetIntRange(0, 1);
	_propOnlyHuman.SetIntRange(0, 1);
	_propOnlyVisible.SetIntRange(0, 1);
	_propTeam.SetIntRange(0, MAX_TEAMS);
	_propRadius.SetFloatRange(0, 100);
	_propRadiusDelta.SetFloatRange(0, 100);
}

int GC_Trigger::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 8;
}

ObjectProperty* GC_Trigger::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
		case 0: return &_propActive;
		case 1: return &_propOnlyHuman;
		case 2: return &_propOnlyVisible;
		case 3: return &_propTeam;
		case 4: return &_propRadius;
		case 5: return &_propRadiusDelta;
		case 6: return &_propOnEnter;
		case 7: return &_propOnLeave;
	}

	_ASSERT(FALSE);
	return NULL;
}

void GC_Trigger::MyPropertySet::Exchange(bool applyToObject)
{
	BASE::Exchange(applyToObject);

	GC_Trigger *tmp = static_cast<GC_Trigger *>(GetObject());

	if( applyToObject )
	{
		_propOnlyHuman.GetIntValue() ? tmp->SetFlags(GC_FLAG_TRIGGER_ONLYHUMAN) : tmp->ClearFlags(GC_FLAG_TRIGGER_ONLYHUMAN);
		_propOnlyVisible.GetIntValue() ? tmp->SetFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) : tmp->ClearFlags(GC_FLAG_TRIGGER_ONLYVISIBLE);
		_propActive.GetIntValue() ? tmp->SetFlags(GC_FLAG_TRIGGER_ACTIVE) : tmp->ClearFlags(GC_FLAG_TRIGGER_ACTIVE);
		tmp->_team = _propTeam.GetIntValue();
		tmp->_radius = _propRadius.GetFloatValue();
		tmp->_radiusDelta = _propRadiusDelta.GetFloatValue();
		tmp->_onEnter = _propOnEnter.GetStringValue();
		tmp->_onLeave = _propOnLeave.GetStringValue();
	}
	else
	{
		_propOnlyHuman.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ONLYHUMAN) ? 1 : 0);
		_propOnlyVisible.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ONLYVISIBLE) ? 1 : 0);
		_propActive.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ACTIVE) ? 1 : 0);
		_propTeam.SetIntValue(tmp->_team);
		_propRadius.SetFloatValue(tmp->_radius);
		_propRadiusDelta.SetFloatValue(tmp->_radiusDelta);
		_propOnEnter.SetStringValue(tmp->_onEnter);
		_propOnLeave.SetStringValue(tmp->_onLeave);
	}
}


// end of file

// Trigger.cpp

#include "stdafx.h"
#include "Trigger.h"
#include "Vehicle.h"

#include "level.h"
#include "Macros.h"


#include "fs/SaveFile.h"
#include "fs/MapFile.h"

IMPLEMENT_SELF_REGISTRATION(GC_Trigger)
{
	ED_ITEM( "trigger", "Триггер" );
	return true;
}


GC_Trigger::GC_Trigger(float x, float y) : GC_2dSprite()
{
	SetTexture("editor_trigger");
	MoveTo(vec2d(x, y));
	SetZ(Z_WOOD);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetFlags(GC_FLAG_TRIGGER_ACTIVE);

	_radius = 1;
}

GC_Trigger::GC_Trigger(FromFile) : GC_2dSprite(FromFile())
{
}

GC_Trigger::~GC_Trigger()
{
}

void GC_Trigger::Serialize(SaveFile &f)
{
	f.Serialize(_radius);
	f.Serialize(_onEnter);
	f.Serialize(_onExit);
	f.Serialize(_veh);
}

void GC_Trigger::mapExchange(MapFile &f)
{
	GC_2dSprite::mapExchange(f);

	MAP_EXCHANGE_FLOAT(radius, _radius, 1);
	MAP_EXCHANGE_STRING(on_enter, _onEnter, "");
	MAP_EXCHANGE_STRING(on_exit, _onExit, "");
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
				float rr = (GetPos() - veh->GetPos()).sqr();
				if( rr < rr_min )
				{
					rr_min = rr;
					_veh = veh;
				}
			}
		}
		if( _veh )
		{
			script_exec(g_env.L, _onEnter.c_str());
		}
	}
	else if( _veh )
	{
		if( _veh->IsKilled() )
		{
			_veh = NULL;
			return;
		}

		if( (GetPos() - _veh->GetPos()).sqr() > _radius*_radius*CELL_SIZE*CELL_SIZE )
		{
			script_exec(g_env.L, _onExit.c_str());
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
  , _propActive( ObjectProperty::TYPE_INTEGER, "active" )
  , _propRadius( ObjectProperty::TYPE_FLOAT, "radius" )
  , _propOnEnter( ObjectProperty::TYPE_STRING, "on_enter" )
  , _propOnExit( ObjectProperty::TYPE_STRING, "on_exit" )
{
	_propActive.SetIntRange(0, 1);
	_propRadius.SetFloatRange(0, 100);
}

int GC_Trigger::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 4;
}

ObjectProperty* GC_Trigger::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propActive;
	case 1: return &_propRadius;
	case 2: return &_propOnEnter;
	case 3: return &_propOnExit;
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
		_propActive.GetIntValue() ? tmp->SetFlags(GC_FLAG_TRIGGER_ACTIVE) : tmp->ClearFlags(GC_FLAG_TRIGGER_ACTIVE);
		tmp->_radius = _propRadius.GetFloatValue();
		tmp->_onEnter = _propOnEnter.GetStringValue();
		tmp->_onExit = _propOnExit.GetStringValue();
	}
	else
	{
		_propActive.SetIntValue(tmp->CheckFlags(GC_FLAG_TRIGGER_ACTIVE) ? 1 : 0);
		_propRadius.SetFloatValue(tmp->_radius);
		_propOnEnter.SetStringValue(tmp->_onEnter);
		_propOnExit.SetStringValue(tmp->_onExit);
	}
}


// end of file

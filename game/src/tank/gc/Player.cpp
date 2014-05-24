// Player.cpp

#include "Player.h"

#include "Camera.h"
#include "GameClasses.h"
#include "indicators.h"
#include "World.h"
#include "Macros.h"
#include "MapFile.h"
#include "particles.h"
#include "SaveFile.h"
#include "Sound.h"
#include "script.h"
#include "vehicle.h"

#include "gui.h"
#include "gui_desktop.h"

//#include "network/TankClient.h"

#include "config/Config.h"
#include "config/Language.h"

#include "video/TextureManager.h"

#include "core/debug.h"

#include <GuiManager.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

IMPLEMENT_SELF_REGISTRATION(GC_Player)
{
    return true;
}

IMPLEMENT_MEMBER_OF(GC_Player, LIST_players);

GC_Player::GC_Player(World &world)
  : _timeRespawn(PLAYER_RESPAWN_DELAY)
  , _team(0)
  , _score(0)
{
	SetEvents(world, GC_FLAG_OBJECT_EVENTS_TS_FIXED);

	// select nick from the random_names table
	lua_getglobal(g_env.L, "random_name");     // push function
	lua_call(g_env.L, 0, 1);                   // call it
	SetNick(lua_tostring(g_env.L, -1));        // get value
	lua_pop(g_env.L, 1);                       // pop result

	// !! avoid using net_rand in constructor since it may cause sync error

	// select first available class
//	int count = 0;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
	//	if( 0 == world.net_rand() % ++count )
		{
			SetClass(lua_tostring(g_env.L, -2));  // get vehicle class
		}
	}
	lua_pop(g_env.L, 1); // remove classes table

	// select the default red skin
	SetSkin("red");
}

GC_Player::GC_Player(FromFile)
{
}

GC_Player::~GC_Player()
{
}

void GC_Player::Serialize(World &world, SaveFile &f)
{
	GC_Service::Serialize(world, f);

	f.Serialize(_scriptOnDie);
	f.Serialize(_scriptOnRespawn);
	f.Serialize(_vehname);
	f.Serialize(_nick);
	f.Serialize(_skin);
	f.Serialize(_class);
	f.Serialize(_score);
	f.Serialize(_team);
	f.Serialize(_timeRespawn);
	f.Serialize(_vehicle);
}

void GC_Player::MapExchange(World &world, MapFile &f)
{
	GC_Service::MapExchange(world, f);
	MAP_EXCHANGE_STRING(on_die, _scriptOnDie, "");
	MAP_EXCHANGE_STRING(on_respawn, _scriptOnRespawn, "");
	MAP_EXCHANGE_STRING(vehname, _vehname, "");
	MAP_EXCHANGE_STRING(nick, _nick, "");
	MAP_EXCHANGE_STRING(skin, _skin, "");
	MAP_EXCHANGE_STRING(class, _class, "");
	MAP_EXCHANGE_INT(score, _score, 0);
	MAP_EXCHANGE_INT(team, _team, 0);
}

void GC_Player::Kill(World &world)
{
	if( _vehicle )
		_vehicle->Kill(world); // the reference is released in the OnVehicleKill()
	GC_Service::Kill(world);
}

void GC_Player::SetSkin(const std::string &skin)
{
	_skin = skin;
	UpdateSkin();
}

void GC_Player::SetNick(const std::string &nick)
{
	_nick = nick;
}

void GC_Player::SetClass(const std::string &c)
{
	_class = c;
}

void GC_Player::SetTeam(int team)
{
	_team = team;
}

void GC_Player::UpdateSkin()
{
	if( _vehicle )
		_vehicle->SetSkin(_skin);
}

void GC_Player::SetScore(World &world, int score)
{
	_score = score;
	if( g_conf.sv_fraglimit.GetInt() )
	{
		if( _score >= g_conf.sv_fraglimit.GetInt() )
		{
			world.HitLimit();
		}
	}
}

void GC_Player::OnRespawn()
{
}

void GC_Player::OnDie()
{
}

void GC_Player::TimeStepFixed(World &world, float dt)
{
	GC_Service::TimeStepFixed(world, dt);

	if( !GetVehicle() )
	{
		_timeRespawn -= dt;
		if( _timeRespawn <= 0 )
		{
			//
			// Respawn
			//

			assert(!GetVehicle());
			_timeRespawn = PLAYER_RESPAWN_DELAY;

			std::vector<GC_SpawnPoint*> points;
			GC_SpawnPoint *pSpawnPoint;

			GC_SpawnPoint *pBestPoint = NULL;
			float max_dist = -1;

			FOREACH( world.GetList(LIST_respawns), GC_SpawnPoint, object )
			{
				pSpawnPoint = (GC_SpawnPoint*) object;
				if( pSpawnPoint->_team && (pSpawnPoint->_team != _team) )
					continue;

				float dist = -1;
				FOREACH( world.GetList(LIST_vehicles), GC_Vehicle, pVeh )
				{
					float d = (pVeh->GetPos() - pSpawnPoint->GetPos()).sqr();
					if( d < dist || dist < 0 ) dist = d;
				}

				if( dist > 0 && dist < 4*CELL_SIZE*CELL_SIZE )
				{
					continue;
				}

				if( dist < 0 || dist > 400*CELL_SIZE*CELL_SIZE )
					points.push_back(pSpawnPoint);

				if( dist > max_dist )
				{
					max_dist = dist;
					pBestPoint = pSpawnPoint;
				}
			}

			if( !pBestPoint && points.empty() )
			{
				char buf[64];
				sprintf(buf, g_lang.msg_no_respawns_for_team_x.Get().c_str(), _team);
				static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(buf);
				return;
			}

			if( !points.empty() )
			{
				pBestPoint = points[world.net_rand() % points.size()];
			}

			(new GC_Text_ToolTip(world, pBestPoint->GetPos(), _nick, "font_default"))->Register(world);


			_vehicle = new GC_Tank_Light(world, pBestPoint->GetPos().x, pBestPoint->GetPos().y);
            _vehicle->Register(world);
			GC_Object* found = world.FindObject(_vehname);
			if( found && _vehicle != found )
			{
				GetConsole().Printf(1, "object with name \"%s\" already exists", _vehname.c_str());
			}
			else
			{
				_vehicle->SetName(world, _vehname.c_str());
			}

			_vehicle->SetDirection(pBestPoint->GetDirection());
			_vehicle->SetDirection(pBestPoint->GetDirection());
			_vehicle->SetPlayer(world, this);

			_vehicle->Subscribe(NOTIFY_RIGIDBODY_DESTROY, this, (NOTIFYPROC) &GC_Player::OnVehicleDestroy);
			_vehicle->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Player::OnVehicleKill);

			_vehicle->ResetClass();

			UpdateSkin();
			OnRespawn();
			if( !_scriptOnRespawn.empty() )
			{
				script_exec(g_env.L, _scriptOnRespawn.c_str());
			}
		}
	}
}

void GC_Player::OnVehicleDestroy(World &world, GC_Object *sender, void *param)
{
	_vehicle->Unsubscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Player::OnVehicleKill);
	_vehicle->Unsubscribe(NOTIFY_RIGIDBODY_DESTROY, this, (NOTIFYPROC) &GC_Player::OnVehicleDestroy);
	_vehicle = NULL;
	OnDie();
	if( !_scriptOnDie.empty() )
	{
		script_exec(g_env.L, _scriptOnDie.c_str());
	}
}

void GC_Player::OnVehicleKill(World &world, GC_Object *sender, void *param)
{
	_vehicle->Unsubscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Player::OnVehicleKill);
	_vehicle->Unsubscribe(NOTIFY_RIGIDBODY_DESTROY, this, (NOTIFYPROC) &GC_Player::OnVehicleDestroy);
	_vehicle = NULL;
	OnDie();
}

PropertySet* GC_Player::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Player::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTeam(      ObjectProperty::TYPE_INTEGER,     "team"    )
  , _propScore(     ObjectProperty::TYPE_INTEGER,     "score"   )
  , _propNick(      ObjectProperty::TYPE_STRING,      "nick"    )
  , _propClass(     ObjectProperty::TYPE_MULTISTRING, "class"   )
  , _propSkin(      ObjectProperty::TYPE_MULTISTRING, "skin"    )
  , _propOnDie(     ObjectProperty::TYPE_STRING,      "on_die"      )
  , _propOnRespawn( ObjectProperty::TYPE_STRING,      "on_respawn"  )
  , _propVehName(   ObjectProperty::TYPE_STRING,      "vehname" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS);
	_propScore.SetIntRange(INT_MIN, INT_MAX);

	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
		// now 'key' is at index -2 and 'value' at index -1
		_propClass.AddItem(lua_tostring(g_env.L, -2));
	}
	lua_pop(g_env.L, 1); // pop classes table

	std::vector<std::string> skin_names;
	g_texman->GetTextureNames(skin_names, "skin/", true);
	for( size_t i = 0; i < skin_names.size(); ++i )
	{
		_propSkin.AddItem( skin_names[i]);
	}
}

int GC_Player::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 8;
}

ObjectProperty* GC_Player::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
		case 0: return &_propTeam;
		case 1: return &_propScore;
		case 2: return &_propNick;
		case 3: return &_propClass;
		case 4: return &_propSkin;
		case 5: return &_propVehName;
		case 6: return &_propOnDie;
		case 7: return &_propOnRespawn;
	}

	assert(false);
	return NULL;
}

void GC_Player::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Player *tmp = static_cast<GC_Player *>(GetObject());

	if( applyToObject )
	{
		tmp->SetTeam( _propTeam.GetIntValue() );
		tmp->SetScore( world, _propScore.GetIntValue() );
		tmp->SetNick( _propNick.GetStringValue() );
		tmp->SetClass( _propClass.GetListValue(_propClass.GetCurrentIndex()) );
		tmp->SetSkin( _propSkin.GetListValue(_propSkin.GetCurrentIndex()) );
		tmp->_scriptOnDie = _propOnDie.GetStringValue();
		tmp->_scriptOnRespawn = _propOnRespawn.GetStringValue();

		if( tmp->GetVehicle() )
		{
			const char *name = _propVehName.GetStringValue().c_str();
			GC_Object* found = world.FindObject(name);
			if( found && tmp->GetVehicle() != found )
			{
				GetConsole().Printf(1, "WARNING: object with name \"%s\" already exists", name);
			}
			else
			{
				tmp->GetVehicle()->SetName(world, name);
				tmp->_vehname = name;
			}
		}
		else
		{
			tmp->_vehname = _propVehName.GetStringValue();
		}
	}
	else
	{
		_propOnRespawn.SetStringValue(tmp->_scriptOnRespawn);
		_propOnDie.SetStringValue(tmp->_scriptOnDie);
		_propTeam.SetIntValue(tmp->GetTeam());
		_propScore.SetIntValue(tmp->GetScore());
		_propNick.SetStringValue(tmp->GetNick());
		_propVehName.SetStringValue(tmp->_vehname);

		for( size_t i = 0; i < _propClass.GetListSize(); ++i )
		{
			if( tmp->GetClass() == _propClass.GetListValue(i) )
			{
				_propClass.SetCurrentIndex(i);
				break;
			}
		}

		for( size_t i = 0; i < _propSkin.GetListSize(); ++i )
		{
			if( tmp->GetSkin() == _propSkin.GetListValue(i) )
			{
				_propSkin.SetCurrentIndex(i);
				break;
			}
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlayerLocal)
{
	ED_SERVICE("player_local", "obj_service_player_local");
	return true;
}

GC_PlayerLocal::GC_PlayerLocal(World &world)
  : GC_Player(world)
{
	(new GC_Camera(world, this))->Register(world);
    GC_Camera::UpdateLayout(world);
}

GC_PlayerLocal::GC_PlayerLocal(FromFile)
  : GC_Player(FromFile())
{
}

GC_PlayerLocal::~GC_PlayerLocal()
{
}

///////////////////////////////////////////////////////////////////////////////
// end of file

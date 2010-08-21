// Player.cpp

#include "stdafx.h"

#include "Player.h"

#include "script.h"
#include "macros.h"
#include "level.h"

#include "KeyMapper.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "network/TankClient.h"

#include "config/Config.h"
#include "config/Language.h"

#include "video/TextureManager.h"

#include "core/debug.h"

#include "ui/GuiManager.h"
#include "ui/gui_desktop.h"
#include "ui/gui.h"


#include "GameClasses.h"
#include "Camera.h"
#include "vehicle.h"
#include "indicators.h"
#include "particles.h"
#include "Sound.h"

//////////////////////////////////////////////////////////////////////////////////////////////

GC_Player::GC_Player()
  : GC_Service()
  , _memberOf(this)
{
	_timeRespawn = PLAYER_RESPAWN_DELAY;

	_team  = 0;
	_score = 0;

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);



	// select nick from the random_names table
	lua_getglobal(g_env.L, "random_name");     // push function
	lua_call(g_env.L, 0, 1);                   // call it
	SetNick(lua_tostring(g_env.L, -1));        // get value
	lua_pop(g_env.L, 1);                       // pop result

	// !! avoid using net_rand in constructor since it may cause sync error

	// select first available class
	int count = 0;
	lua_getglobal(g_env.L, "classes");
	for( lua_pushnil(g_env.L); lua_next(g_env.L, -2); lua_pop(g_env.L, 1) )
	{
	//	if( 0 == g_level->net_rand() % ++count )
		{
			SetClass(lua_tostring(g_env.L, -2));  // get vehicle class
		}
	}
	lua_pop(g_env.L, 1); // remove classes table

	// select the default red skin
	SetSkin("red");
}

GC_Player::GC_Player(FromFile)
  : GC_Service(FromFile())
  , _memberOf(this)
{
}

GC_Player::~GC_Player()
{
}

void GC_Player::Serialize(SaveFile &f)
{
	GC_Service::Serialize(f);

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

void GC_Player::MapExchange(MapFile &f)
{
	GC_Service::MapExchange(f);
	MAP_EXCHANGE_STRING(on_die, _scriptOnDie, "");
	MAP_EXCHANGE_STRING(on_respawn, _scriptOnRespawn, "");
	MAP_EXCHANGE_STRING(vehname, _vehname, "");
	MAP_EXCHANGE_STRING(nick, _nick, "");
	MAP_EXCHANGE_STRING(skin, _skin, "");
	MAP_EXCHANGE_STRING(class, _class, "");
	MAP_EXCHANGE_INT(score, _score, 0);
	MAP_EXCHANGE_INT(team, _team, 0);
}

void GC_Player::Kill()
{
	if( _vehicle )
		_vehicle->Kill(); // the reference is released in the OnVehicleKill()
	GC_Service::Kill();
}

void GC_Player::SetSkin(const string_t &skin)
{
	_skin = skin;
	UpdateSkin();
}

void GC_Player::SetNick(const string_t &nick)
{
	_nick = nick;
}

void GC_Player::SetClass(const string_t &c)
{
	_class = c;
	if (_vehicle != NULL)
	_vehicle->ResetClass();
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

void GC_Player::SetVehicle(const SafePtr<GC_Vehicle> &car)
{
	_vehicle = car;
	if (car != NULL && car->GetName() != NULL)
	_vehname=car->GetName();
}

void GC_Player::SetScore(int score)
{
	_score = score;
	if( g_conf.sv_fraglimit.GetInt() )
	{
		if( _score >= g_conf.sv_fraglimit.GetInt() )
		{
			g_level->HitLimit();
		}
	}
}

void GC_Player::OnRespawn()
{
}

void GC_Player::OnDie()
{
}

void GC_Player::TimeStepFixed(float dt)
{
	GC_Service::TimeStepFixed( dt );

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

			FOREACH( g_level->GetList(LIST_respawns), GC_SpawnPoint, object )
			{
				pSpawnPoint = (GC_SpawnPoint*) object;
				if( pSpawnPoint->_team && (pSpawnPoint->_team != _team) )
					continue;

				float dist = -1;
				FOREACH( g_level->GetList(LIST_vehicles), GC_Vehicle, pVeh )
				{
					if( pVeh->IsKilled() ) continue;
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
				wsprintf(buf, g_lang.msg_no_respawns_for_team_x.Get().c_str(), _team);
				static_cast<UI::Desktop*>(g_gui->GetDesktop())->GetMsgArea()->WriteLine(buf);
				return;
			}

			if( !points.empty() )
			{
				pBestPoint = points[g_level->net_rand() % points.size()];
			}

			new GC_Text_ToolTip(pBestPoint->GetPos(), _nick, "font_default");


			_vehicle = /*WrapRawPtr*/(new GC_Tank_Light(pBestPoint->GetPos().x, pBestPoint->GetPos().y));
			GC_Object* found = g_level->FindObject(_vehname);
			if( found && _vehicle != found )
			{
				GetConsole().Printf(1, "object with name \"%s\" already exists", _vehname.c_str());
			}
			else
			{
				_vehicle->SetName(_vehname.c_str());
			}
			_vehicle->GetVisual()->SetMoveSound(SND_TankMove);
			_vehicle->SetDirection(pBestPoint->GetDirection());
			_vehicle->GetVisual()->SetDirection(pBestPoint->GetDirection());
			_vehicle->SetPlayer(WrapRawPtr(this));

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

void GC_Player::OnVehicleDestroy(GC_Object *sender, void *param)
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

void GC_Player::Unsubscribed()
{
	//PulseNotify(NOTIFY_OBJECT_KILL);
	if (_vehicle)
	{
	_vehicle->Unsubscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Player::OnVehicleKill);
	_vehicle->Unsubscribe(NOTIFY_RIGIDBODY_DESTROY, this, (NOTIFYPROC) &GC_Player::OnVehicleDestroy);
	}
	//_vehicle = NULL;
}

void GC_Player::Subscribed()
{
	if (_vehicle)
	{
	_vehicle->Subscribe(NOTIFY_RIGIDBODY_DESTROY, this, (NOTIFYPROC) &GC_Player::OnVehicleDestroy);
	_vehicle->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Player::OnVehicleKill);
	}
}

void GC_Player::OnVehicleKill(GC_Object *sender, void *param)
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
  , _propVehName(   ObjectProperty::TYPE_STRING,      "vehname" )
  , _propOnDie(     ObjectProperty::TYPE_STRING,      "on_die"      )
  , _propOnRespawn( ObjectProperty::TYPE_STRING,      "on_respawn"  )
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

	std::vector<string_t> skin_names;
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

void GC_Player::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_Player *tmp = static_cast<GC_Player *>(GetObject());

	if( applyToObject )
	{
		tmp->SetTeam( _propTeam.GetIntValue() );
		tmp->SetScore( _propScore.GetIntValue() );
		tmp->SetNick( _propNick.GetStringValue() );
		tmp->SetClass( _propClass.GetListValue(_propClass.GetCurrentIndex()) );
		tmp->SetSkin( _propSkin.GetListValue(_propSkin.GetCurrentIndex()) );
		tmp->_scriptOnDie = _propOnDie.GetStringValue();
		tmp->_scriptOnRespawn = _propOnRespawn.GetStringValue();

		if( tmp->GetVehicle() )
		{
			const char *name = _propVehName.GetStringValue().c_str();
			GC_Object* found = g_level->FindObject(name);
			if( found && tmp->GetVehicle() != found )
			{
				GetConsole().Printf(1, "WARNING: object with name \"%s\" already exists", name);
			}
			else
			{
				tmp->GetVehicle()->SetName(name);
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

GC_PlayerHuman::GC_PlayerHuman()
{
	ZeroMemory(&_ctrlState, sizeof(_ctrlState));
}

GC_PlayerHuman::GC_PlayerHuman(FromFile)
  : GC_Player(FromFile())
{
	ZeroMemory(&_ctrlState, sizeof(_ctrlState));
}

GC_PlayerHuman::~GC_PlayerHuman()
{
}

void GC_PlayerHuman::SetControllerState(const VehicleState &vs)
{
	_ctrlState = vs;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlayerLocal)
{
	ED_SERVICE("player_local", "obj_service_player_local");
	return true;
}

GC_PlayerLocal::GC_PlayerLocal()
  : _lastLightKeyState(false)
  , _lastLightsState(true)
{
	new GC_Camera(WrapRawPtr(this));
	SelectFreeProfile();
}

GC_PlayerLocal::GC_PlayerLocal(FromFile)
  : GC_PlayerHuman(FromFile())
{
}

GC_PlayerLocal::~GC_PlayerLocal()
{
}

void GC_PlayerLocal::SelectFreeProfile()
{
	// find first available profile that is not used by another player
	string_t profile;

	std::vector<string_t> tmp;
	g_conf.dm_profiles.GetKeyList(tmp);
	for( size_t i = 0; i < tmp.size(); ++i )
	{
		bool in_use = false;
		FOREACH(g_level->GetList(LIST_players), GC_Player, player)
		{
			if( !player->IsKilled() )
			{
				if( GC_PlayerLocal *p = dynamic_cast<GC_PlayerLocal *>(player) )
				{
					if( p->GetProfile() == tmp[i] )
					{
						in_use = true;
						break;
					}
				}
			}
		}
		if( !in_use )
		{
			SetProfile(tmp[i]);
			return;
		}
	}
	SetProfile(""); // there was no available profile found
}

void GC_PlayerLocal::Serialize(SaveFile &f)
{
	GC_PlayerHuman::Serialize(f);
	f.Serialize(_profile);
	if( f.loading() )
	{
		SetProfile(_profile);
		_lastLightKeyState = false;
	}
}

void GC_PlayerLocal::MapExchange(MapFile &f)
{
	GC_PlayerHuman::MapExchange(f);
	MAP_EXCHANGE_STRING(profile, _profile, "");
	if( f.loading() )
	{
		SetProfile(_profile);
		_lastLightKeyState = false;
	}
}

unsigned short GC_PlayerLocal::GetNetworkID() const
{
	assert(g_client);
	return g_client->GetId();
}

void GC_PlayerLocal::TimeStepFixed(float dt)
{
	GC_PlayerHuman::TimeStepFixed( dt );

	assert(!_stateHistory.empty());
	_stateHistory.pop_front();

	if( GetVehicle() )
	{
		GetVehicle()->SetState(_ctrlState);
		GetVehicle()->TimeStepFixed(dt); // vehicle may die here
	}

	if( GetVehicle() )
	{
		GC_RigidBodyDynamic::PushState();
		GetVehicle()->GetVisual()->SetFlags(GC_FLAG_VEHICLEDUMMY_TRACKS, false);
		GetVehicle()->GetVisual()->Sync(GetVehicle());
		for( std::deque<VehicleState>::const_iterator it = _stateHistory.begin(); it != _stateHistory.end(); ++it )
		{
			GetVehicle()->SetPredictedState(*it);
			GetVehicle()->GetVisual()->TimeStepFixed(dt);
			GC_RigidBodyDynamic::ProcessResponse(dt);
		}
		GetVehicle()->GetVisual()->SetFlags(GC_FLAG_VEHICLEDUMMY_TRACKS, true);
		GC_RigidBodyDynamic::PopState();
	}
}

const string_t& GC_PlayerLocal::GetProfile() const
{
	return _profile;
}

void GC_PlayerLocal::SetProfile(const string_t &name)
{
	_profile = name;
	ConfVar *p = g_conf.dm_profiles.Find(name);

	if( p && ConfVar::typeTable == p->GetType() )
	{
		ConfControllerProfile t(p->AsTable());

		_keyForward     = GetKeyCode(t.key_forward.Get());
		_keyBack        = GetKeyCode(t.key_back.Get());
		_keyLeft        = GetKeyCode(t.key_left.Get());
		_keyRight       = GetKeyCode(t.key_right.Get());
		_keyFire        = GetKeyCode(t.key_fire.Get());
		_keyLight       = GetKeyCode(t.key_light.Get());
		_keyTowerLeft   = GetKeyCode(t.key_tower_left.Get());
		_keyTowerRight  = GetKeyCode(t.key_tower_right.Get());
		_keyTowerCenter = GetKeyCode(t.key_tower_center.Get());
		_keyPickup      = GetKeyCode(t.key_pickup.Get());

		_lastLightsState = t.lights.Get();
		_aimToMouse = t.aim_to_mouse.Get();
		_moveToMouse = t.move_to_mouse.Get();
		_arcadeStyle = t.arcade_style.Get();
	}
	else
	{
		TRACE("WARNING: profile '%s' not found", name.c_str());

		_keyForward     = 0;
		_keyBack        = 0;
		_keyLeft        = 0;
		_keyRight       = 0;
		_keyFire        = 0;
		_keyLight       = 0;
		_keyTowerLeft   = 0;
		_keyTowerRight  = 0;
		_keyTowerCenter = 0;
		_keyPickup      = 0;

		_lastLightsState  = true;
		_aimToMouse       = false;
		_moveToMouse      = false;
		_arcadeStyle      = false;
	}
}

void GC_PlayerLocal::ReadControllerStateAndStepPredicted(VehicleState &vs, float dt)
{
	ZeroMemory(&vs, sizeof(VehicleState));
	if( GetVehicle() )
	{
		//
		// lights
		//
		bool tmp = g_env.envInputs.IsKeyPressed(_keyLight);
		if( tmp && !_lastLightKeyState && g_conf.sv_nightmode.Get() )
		{
			PLAY(SND_LightSwitch, GetVehicle()->GetPos());
			_lastLightsState = !_lastLightsState;
		}
		_lastLightKeyState = tmp;
		vs._bLight = _lastLightsState;


		//
		// pickup
		//
		vs._bState_AllowDrop = g_env.envInputs.IsKeyPressed(_keyPickup)
			|| ( g_env.envInputs.IsKeyPressed(_keyForward) && g_env.envInputs.IsKeyPressed(_keyBack)  )
			|| ( g_env.envInputs.IsKeyPressed(_keyLeft)    && g_env.envInputs.IsKeyPressed(_keyRight) );

		//
		// fire
		//
		vs._bState_Fire = g_env.envInputs.IsKeyPressed(_keyFire);


		//
		// movement
		//
		if( _arcadeStyle )
		{
			vec2d tmp(0, 0);
			if( g_env.envInputs.IsKeyPressed(_keyForward) ) tmp.y -= 1;
			if( g_env.envInputs.IsKeyPressed(_keyBack)    ) tmp.y += 1;
			if( g_env.envInputs.IsKeyPressed(_keyLeft)    ) tmp.x -= 1;
			if( g_env.envInputs.IsKeyPressed(_keyRight)   ) tmp.x += 1;
			tmp.Normalize();

			bool move = tmp.x || tmp.y;
			bool sameDirection = tmp * GetVehicle()->GetDirection() > cos(PI/4);

			bool bBack = move && !sameDirection && NULL != g_level->TraceNearest(g_level->grid_rigid_s, GetVehicle(), 
				GetVehicle()->GetPos(), GetVehicle()->GetDirection() * GetVehicle()->GetRadius());
			bool bForv = move && !bBack;

			vs._bState_MoveForward = sameDirection && bForv;
			vs._bState_MoveBack = bBack;
			vs._bExplicitBody = move;
			vs._fBodyAngle = tmp.Angle();
		}
		else
		{
			vs._bState_MoveForward = g_env.envInputs.IsKeyPressed(_keyForward);
			vs._bState_MoveBack    = g_env.envInputs.IsKeyPressed(_keyBack   );
			vs._bState_RotateLeft  = g_env.envInputs.IsKeyPressed(_keyLeft   );
			vs._bState_RotateRight = g_env.envInputs.IsKeyPressed(_keyRight  );
		}

		if( _moveToMouse )
		{
			vs._bState_Fire = vs._bState_Fire || g_env.envInputs.bLButtonState;
			vs._bState_AllowDrop = vs._bState_AllowDrop || g_env.envInputs.bMButtonState;

			vec2d pt;
			if( g_env.envInputs.bRButtonState && GC_Camera::GetWorldMousePos(pt) )
			{
				vec2d tmp = pt - GetVehicle()->GetPos() - GetVehicle()->GetBrakingLength();
				if( tmp.sqr() > 1 )
				{
					if( tmp * GetVehicle()->GetDirection() < 0 )
					{
						tmp = -tmp;
						vs._bState_MoveBack    = true;
					}
					else
					{
						vs._bState_MoveForward = true;
					}
					vs._bExplicitBody = true;
					vs._fBodyAngle = tmp.Angle();
				}
			}
		}



		//
		// tower control
		//
		if( _aimToMouse )
		{
			vs._bState_Fire = vs._bState_Fire || g_env.envInputs.bLButtonState;
			if( !_moveToMouse )
			{
				vs._bState_AllowDrop = vs._bState_AllowDrop || g_env.envInputs.bRButtonState;
			}

			vec2d pt;
			if( GetVehicle() && GetVehicle()->GetWeapon() && GC_Camera::GetWorldMousePos(pt) )
			{
				float a = (pt - GetVehicle()->GetPos()).Angle();
				vs._bExplicitTower = true;
				vs._fTowerAngle = a - GetVehicle()->GetDirection().Angle() - GetVehicle()->GetSpinup();
			}
		}
		else
		{
			vs._bState_TowerLeft   = g_env.envInputs.IsKeyPressed(_keyTowerLeft);
			vs._bState_TowerRight  = g_env.envInputs.IsKeyPressed(_keyTowerRight);
			vs._bState_TowerCenter = g_env.envInputs.IsKeyPressed(_keyTowerCenter)
				|| g_env.envInputs.IsKeyPressed(_keyTowerLeft) && g_env.envInputs.IsKeyPressed(_keyTowerRight);
			if( vs._bState_TowerCenter )
			{
				vs._bState_TowerLeft  = false;
				vs._bState_TowerRight = false;
			}
		}

	}

	//
	// step predicted
	//

	VehicleState vs1;
	ControlPacket cp;
	cp.fromvs(vs);
	cp.tovs(vs1);
	_stateHistory.push_back(vs1);

	if( GetVehicle() )
	{
		GC_RigidBodyDynamic::PushState();
		GetVehicle()->SetPredictedState(vs1);
		GetVehicle()->GetVisual()->TimeStepFixed(dt);
		GC_RigidBodyDynamic::ProcessResponse(dt);
		GC_RigidBodyDynamic::PopState();
	}
}

PropertySet* GC_PlayerLocal::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_PlayerLocal::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propProfile( ObjectProperty::TYPE_STRING, "profile"  )
{
}

int GC_PlayerLocal::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_PlayerLocal::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
		case 0: return &_propProfile;
	}

	assert(false);
	return NULL;
}

void GC_PlayerLocal::MyPropertySet::MyExchange(bool applyToObject)
{
	BASE::MyExchange(applyToObject);

	GC_PlayerLocal *tmp = static_cast<GC_PlayerLocal *>(GetObject());

	if( applyToObject )
	{
		tmp->SetProfile(_propProfile.GetStringValue());
	}
	else
	{
		_propProfile.SetStringValue(tmp->_profile);
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlayerRemote)
{
	return true;
}

GC_PlayerRemote::GC_PlayerRemote(unsigned short id)
{
	_networkId = id;
}

GC_PlayerRemote::GC_PlayerRemote(FromFile)
  : GC_PlayerHuman(FromFile())
  , _networkId(-1)
{
}

GC_PlayerRemote::~GC_PlayerRemote()
{
}

void GC_PlayerRemote::Serialize(SaveFile &f)
{
	GC_PlayerHuman::Serialize(f);
	f.Serialize(_networkId);
}

void GC_PlayerRemote::TimeStepFixed(float dt)
{
	GC_PlayerHuman::TimeStepFixed( dt );
	assert(g_client);
	if( GetVehicle() )
	{
		GetVehicle()->SetState(_ctrlState);
		GetVehicle()->GetVisual()->Sync(GetVehicle()); // FIXME
	//	GetVehicle()->SetPredictedState(vs);
	//	GetVehicle()->GetVisual()->TimeStepFixed(dt);
	}
}



///////////////////////////////////////////////////////////////////////////////
// end of file

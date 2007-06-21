// Player.cpp

#include "stdafx.h"

#include "Player.h"

#include "macros.h"
#include "level.h"

#include "KeyMapper.h"

#include "fs/SaveFile.h"

#include "network/TankClient.h"

#include "config/Config.h"

#include "core/Console.h"
#include "core/debug.h"

#include "GameClasses.h"
#include "Camera.h"
#include "vehicle.h"
#include "indicators.h"
#include "particles.h"


//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Player)
{
	return true;
}

GC_Player::GC_Player()
  : GC_Service(), _memberOf(g_level->players, this)
{
	_timeRespawn = PLAYER_RESPAWNTIME;

	_team  = 0;
	_score = 0;
	_nick  = "player";

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Player::GC_Player(FromFile)
  : GC_Service(FromFile()), _memberOf(g_level->players, this)
{
}

GC_Player::~GC_Player()
{
}

void GC_Player::Serialize(SaveFile &f)
{
	GC_Service::Serialize(f);

	f.Serialize(_nick);
	f.Serialize(_skin);
	f.Serialize(_class);
	f.Serialize(_score);
	f.Serialize(_team);
	f.Serialize(_timeRespawn);
	f.Serialize(_vehicle);
}

void GC_Player::Kill()
{
	if( _vehicle )
		_vehicle->Kill();	// объект будет освобожден в OnVehicleKill()
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
}

void GC_Player::SetTeam(int team)
{
	_team = team;
}

void GC_Player::UpdateSkin()
{
	if( _vehicle )
		_vehicle->SetSkin(_skin.c_str());
}

void GC_Player::ResetClass()
{
	VehicleClass vc;

	lua_State *L = g_env.L;
	lua_pushcfunction(L, luaT_ConvertVehicleClass); // function to call
	lua_getglobal(L, "getvclass");
	lua_pushstring(L, _class.c_str()); // cls arg
	if( lua_pcall(L, 1, 1, 0) )
	{
		// print error message
		_MessageArea::Inst()->message(lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	lua_pushlightuserdata(L, &vc);
	if( lua_pcall(L, 2, 0, 0) )
	{
		// print error message
		_MessageArea::Inst()->message(lua_tostring(L, -1));
		lua_pop(L, 1);
		return;
	}

	_vehicle->SetClass(vc);
}

void GC_Player::ChangeScore(int delta)
{
	_score += delta;
	if( g_conf.sv_fraglimit->GetInt() )
	{
		if( _score >= g_conf.sv_fraglimit->GetInt() )
		{
			g_level->Pause(true);
			g_level->_limitHit = true;
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

	if( IsDead() )
	{
		_timeRespawn -= dt;
		if( _timeRespawn <= 0 )
		{
			//
			// Respawn
			//

			_ASSERT(IsDead());
			_timeRespawn = PLAYER_RESPAWNTIME;

			std::vector<GC_SpawnPoint*> points;
			GC_SpawnPoint *pSpawnPoint;

			GC_SpawnPoint *pBestPoint = NULL; // оптимальная точка
			float max_dist = -1;

			FOREACH( respawns, GC_SpawnPoint, object )
			{
				pSpawnPoint = (GC_SpawnPoint*) object;
				if( pSpawnPoint->_team && (pSpawnPoint->_team != _team) )
					continue;

				float dist = -1;
				FOREACH( vehicles, GC_Vehicle, pVeh )
				{
					if( pVeh->IsKilled() ) continue;
					float d = (pVeh->GetPos() - pSpawnPoint->GetPos()).Square();
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
				wsprintf(buf, "Для команды %d нет точек рождения!", _team);
				_MessageArea::Inst()->message(buf);
				return;
			}

			if( !points.empty() )
			{
				pBestPoint = points[g_level->net_rand() % points.size()];
			}

			new GC_Text_ToolTip(pBestPoint->GetPos(), _nick.c_str(), "font_default");


			//if( !dynamic_cast<AIController*>(_controller) )
			//{
			//	static const TextureCache tex1("particle_1");
			//	for( int n = 0; n < 64; ++n )
			//	{
			//		vec2d a(PI2 * (float) n / 64);
			//		new GC_Particle(pBestPoint->GetPos() + a * 28, a * 28, tex1, frand(0.5f) + 0.1f);
			//	}
			//}

			_vehicle = new GC_Tank_Light(pBestPoint->GetPos().x, pBestPoint->GetPos().y);
			_vehicle->SetBodyAngle(pBestPoint->GetRotation());
			_vehicle->SetPlayer(this);

			_vehicle->Subscribe(NOTIFY_OBJECT_KILL, this,
				(NOTIFYPROC) &GC_Player::OnVehicleKill, true, false);
			ResetClass();

			UpdateSkin();
			OnRespawn();
		}
	}
}

void GC_Player::OnVehicleKill(GC_Object *sender, void *param)
{
	OnDie();
	_vehicle = NULL;
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlayerLocal)
{
	return true;
}

GC_PlayerLocal::GC_PlayerLocal()
{
	new GC_Camera(this);
}

GC_PlayerLocal::GC_PlayerLocal(FromFile)
{
}

GC_PlayerLocal::~GC_PlayerLocal()
{
}

void GC_PlayerLocal::Serialize(SaveFile &f)
{
	GC_Player::Serialize(f);
	f.Serialize(_profile);
	if( f.loading() )
	{
		SetProfile(_profile.c_str());
	}
}

void GC_PlayerLocal::TimeStepFixed(float dt)
{
	GC_Player::TimeStepFixed( dt );

	ControlPacket cp;

#ifdef NETWORK_DEBUG
	cp.checksum = g_level->_dwChecksum;
#endif

	if( IsDead() )
	{
		if( g_level->_client )
		{
			g_level->_client->SendControl(cp);
			bool ok = g_level->_client->RecvControl(ControlPacket());
			_ASSERT(ok);
		}
	}
	else
	{
		VehicleState vs;
		GetControl(vs);

		if( g_level->_client  )
		{
			cp.fromvs(vs);
			g_level->_client->SendControl(cp);
			bool ok = g_level->_client->RecvControl(cp);
			_ASSERT(ok);
			cp.tovs(vs);
		}

		GetVehicle()->SetState(vs);
	}
}

void GC_PlayerLocal::SetProfile(const char *name)
{
	_profile = name;
	ConfVar *p = g_conf.dm_profiles->Find(name);

	if( p && ConfVar::typeTable == p->GetType() )
	{
		ConfVarTable *t = static_cast<ConfVarTable *>(p);

		_keyForward     = g_keys->GetCode(t->GetStr( "key_forward"      )->Get());
		_keyBack        = g_keys->GetCode(t->GetStr( "key_back"         )->Get());
		_keyLeft        = g_keys->GetCode(t->GetStr( "key_left"         )->Get());
		_keyRight       = g_keys->GetCode(t->GetStr( "key_right"        )->Get());
		_keyFire        = g_keys->GetCode(t->GetStr( "key_fire"         )->Get());
		_keyLight       = g_keys->GetCode(t->GetStr( "key_light"        )->Get());
		_keyTowerLeft   = g_keys->GetCode(t->GetStr( "key_tower_left"   )->Get());
		_keyTowerRight  = g_keys->GetCode(t->GetStr( "key_tower_right"  )->Get());
		_keyTowerCenter = g_keys->GetCode(t->GetStr( "key_tower_center" )->Get());
		_keyPickup      = g_keys->GetCode(t->GetStr( "key_pickup"       )->Get());
	}
	else
	{
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

		TRACE("WARNING: profile '%s' not found\n", name);
	}
}

void GC_PlayerLocal::GetControl(VehicleState &vs)
{
	ZeroMemory(&vs, sizeof(VehicleState));

	vs._bState_MoveForward = g_env.envInputs.keys[_keyForward] != 0;
	vs._bState_MoveBack    = g_env.envInputs.keys[_keyBack   ] != 0;
	vs._bState_RotateLeft  = g_env.envInputs.keys[_keyLeft   ] != 0;
	vs._bState_RotateRight = g_env.envInputs.keys[_keyRight  ] != 0;
	vs._bState_Fire        = g_env.envInputs.keys[_keyFire   ] != 0;

//	bool tmp = g_env.envInputs.keys[_keyLight] != 0;
//	if( tmp && !_last_light_key_state )
//	{
//		PLAY(SND_LightSwitch, _player->GetVehicle()->GetPos());
//		_bLight = !_bLight;
//	}
//	_last_light_key_state = tmp;
//	vs._bLight = _bLight;

	vs._bState_AllowDrop = 
		(g_env.envInputs.keys[_keyForward] != 0 && g_env.envInputs.keys[_keyBack] != 0) ||
		(g_env.envInputs.keys[_keyLeft] != 0 && g_env.envInputs.keys[_keyRight] != 0) ||
		g_env.envInputs.keys[_keyPickup] != 0;

	vs._bState_TowerLeft   = g_env.envInputs.keys[_keyTowerLeft  ] != 0;
	vs._bState_TowerRight  = g_env.envInputs.keys[_keyTowerRight ] != 0;

	vs._bState_TowerCenter = g_env.envInputs.keys[_keyTowerCenter] != 0 ||
		g_env.envInputs.keys[_keyTowerLeft] != 0 && g_env.envInputs.keys[_keyTowerRight] != 0;

	if( vs._bState_TowerCenter )
	{
		vs._bState_TowerLeft  = 0;
		vs._bState_TowerRight = 0;
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_PlayerRemote)
{
	return true;
}

GC_PlayerRemote::GC_PlayerRemote()
{
	_networkId = 0;
}

GC_PlayerRemote::GC_PlayerRemote(FromFile)
{
}

GC_PlayerRemote::~GC_PlayerRemote()
{
}

void GC_PlayerRemote::Serialize(SaveFile &f)
{
	GC_Player::Serialize(f);
	f.Serialize(_networkId);
}

void GC_PlayerRemote::TimeStepFixed(float dt)
{
	_ASSERT(g_level->_client);

	GC_Player::TimeStepFixed( dt );
	
	if( IsDead() )
	{
		bool ok = g_level->_client->RecvControl(ControlPacket());
		_ASSERT(ok);
	}
	else
	{
		ControlPacket cp;
		bool ok = g_level->_client->RecvControl(cp);
		_ASSERT(ok);

		VehicleState vs;
		cp.tovs(vs);
		GetVehicle()->SetState(vs);
	}
}



///////////////////////////////////////////////////////////////////////////////
// end of file

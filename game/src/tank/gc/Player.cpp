// Player.cpp

#include "stdafx.h"
#include "Player.h"

#include "macros.h"
#include "level.h"
#include "functions.h"
#include "options.h"

#include "fs/SaveFile.h"

#include "network/TankClient.h"


#include "ai.h"
#include "GameClasses.h"
#include "vehicle.h"
#include "controller.h"
#include "indicators.h"
#include "particles.h"

//////////////////////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_Player)
{
	return true;
}

GC_Player::GC_Player(int nTeam)
  : GC_Object(), _memberOf(g_level->players, this)
{
	_time_respawn = PLAYER_RESPAWNTIME;
	_controller  = NULL;

	_networkId = 0;

	_team  = nTeam;
	_score = 0;
	_name  = "player";

	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FIXED);
}

GC_Player::GC_Player(FromFile)
  : GC_Object(FromFile()), _memberOf(g_level->players, this)
{
}

GC_Player::~GC_Player()
{
	_ASSERT(!_controller);
}

void GC_Player::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Object::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_name);
	f.Serialize(_skin);
	f.Serialize(_class);
	/////////////////////////////////////
	f.Serialize(_networkId);
	f.Serialize(_nIndex);
	f.Serialize(_score);
	f.Serialize(_team);
	f.Serialize(_time_respawn);
	/////////////////////////////////////
	f.Serialize(_vehicle);
	/////////////////////////////////////
	if( f.loading() )
		_controller = CreateController(_nIndex);
}

void GC_Player::Kill()
{
	if( _vehicle )
		_vehicle->Kill();	// объект будет освобожден в OnVehicleKill()

	SAFE_DELETE(_controller);

	GC_Object::Kill();
}

void GC_Player::UpdateSkin()
{
	if( _vehicle )
		_vehicle->SetSkin(_skin.c_str());
}

void GC_Player::ResetClass()
{
	VehicleClass vc;

	lua_State *L = LS(g_env.hScript);
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

void GC_Player::Respawn()
{
	_ASSERT(_controller);
	_ASSERT(dead());

	_time_respawn = PLAYER_RESPAWNTIME;

	std::vector<GC_SpawnPoint*> points;
	GC_SpawnPoint *pSpawnPoint;

	GC_SpawnPoint *pBestPoint = NULL; // оптимальная точка
	float max_dist = -1;

	ENUM_BEGIN(respawns, GC_SpawnPoint, object)
	{
		pSpawnPoint = (GC_SpawnPoint*) object;
		if( pSpawnPoint->_team && (pSpawnPoint->_team != _team) )
			continue;

		float dist = -1;
		ENUM_BEGIN(vehicles, GC_Vehicle, pVeh)
		{
			if( pVeh->IsKilled() ) continue;
			float d = (pVeh->_pos - pSpawnPoint->_pos).Square();
			if( d < dist || dist < 0 ) dist = d;
		} ENUM_END();

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
	} ENUM_END();

	if( !pBestPoint && points.empty() )
	{
		char buf[64];
		wsprintf(buf, "Для команды %d нет точек рождения!", _team);
		_MessageArea::Inst()->message(buf);
		return;
	}

	if( !points.empty() )
	{
		pBestPoint = points[net_rand() % points.size()];
	}

	new GC_Text_ToolTip(pBestPoint->_pos, _name.c_str(), "font_default");


	if( OPT(bParticles) )
	{
		if( !dynamic_cast<AIController*>(_controller) )
		{
			static const TextureCache tex1("particle_1");
			for( int n = 0; n < 64; ++n )
			{
				vec2d a(PI2 * (float) n / 64);
				new GC_Particle(pBestPoint->_pos + a * 28, a * 28, tex1, frand(0.5f) + 0.1f);
			}
		}
	}


	_vehicle = new GC_Tank_Light(pBestPoint->_pos, pBestPoint->GetRotation(), this);
	_vehicle->Subscribe(NOTIFY_OBJECT_KILL, this,
		(NOTIFYPROC) &GC_Player::OnVehicleKill, true, false);
	ResetClass();

	_controller->OnPlayerRespawn();

	UpdateSkin();
}

CController* GC_Player::CreateController(int index)
{
	if( OPT(players[index].bAI) )
	{
		return new AIController(this);
	}
	else
	{
		switch( g_options.players[index].ControlType )
		{
		case CT_USER_KB:
			return new CKeyboardController(this, g_options.players[index].KeyMap);
		case CT_USER_KB2:
			return new CKeyboardController_v2(this, g_options.players[index].KeyMap);
		case CT_USER_MOUSE:
			return new CMouseController(this);
		case CT_USER_MOUSE2:
			return new CMouseController2(this);
		case CT_USER_HYBRID:
			return new CControllerHybrid(this, g_options.players[index].KeyMap);
		}
	}
	_ASSERT(FALSE);
    return NULL;
}

void GC_Player::SetController(int nIndex)
{
	SAFE_DELETE(_controller);

	_controller = CreateController(nIndex);
	_nIndex      = nIndex;

	PulseNotify(NOTIFY_PLAYER_SETCONTROLLER);

	if( !g_options.players[_nIndex].bAI && (!g_level->_client ||
		_networkId == g_level->_client->GetId()) )
	{
		new GC_Camera(this);
	}
}

void GC_Player::TimeStepFixed(float dt)
{
	GC_Object::TimeStepFixed( dt );

	ControlPacket cp;

#ifdef NETWORK_DEBUG
	cp.checksum = g_level->_dwChecksum;
#endif


	if( dead() )
	{
		if( g_level->_client && _nIndex < MAX_HUMANS )
		{
			if( g_level->_client->GetId() == _networkId )
				g_level->_client->SendControl(cp);

			bool ok = g_level->_client->RecvControl(ControlPacket());
			_ASSERT(ok);
		}

		_time_respawn -= dt;
		if( _time_respawn <= 0 )
		{
			Respawn();
		}
	}
	else
	{
		_ASSERT(_vehicle);
		_ASSERT(!_vehicle->IsKilled());
		if( _controller )
		{
			VehicleState	vs;
			_controller->SetupVehicleState(&vs, dt);

			if( NULL != g_level->_client && _nIndex < MAX_HUMANS )
			{
				if( g_level->_client->GetId() == _networkId )
				{
					cp.fromvs(vs);
					g_level->_client->SendControl(cp);
				}

				bool ok = g_level->_client->RecvControl(cp);
				_ASSERT(ok);
				cp.tovs(vs);
			}

			_vehicle->SetState(&vs);
		}
	}
}

void GC_Player::OnVehicleKill(GC_Object *sender, void *param)
{
	_controller->Reset();
	_controller->OnPlayerDie();
	_vehicle = NULL;
}

///////////////////////////////////////////////////////////////////////////////
// end of file

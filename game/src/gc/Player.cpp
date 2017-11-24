#include "TypeReg.h"
#include "inc/gc/Player.h"
#include "inc/gc/GameClasses.h"
#include "inc/gc/Indicators.h"
#include "inc/gc/Macros.h"
#include "inc/gc/Particles.h"
#include "inc/gc/Pickup.h"
#include "inc/gc/TypeSystem.h"
#include "inc/gc/Vehicle.h"
#include "inc/gc/VehicleClasses.h"
#include "inc/gc/World.h"
#include "inc/gc/WorldCfg.h"
#include "inc/gc/WorldEvents.h"
#include "inc/gc/SaveFile.h"
#include <MapFile.h>
#include <climits>

IMPLEMENT_SELF_REGISTRATION(GC_Player)
{
	ED_SERVICE("player", "obj_service_player");
	return true;
}

IMPLEMENT_2LIST_MEMBER(GC_Player, LIST_players, LIST_timestep);

GC_Player::GC_Player()
  : _timeRespawn(PLAYER_RESPAWN_DELAY)
  , _team(0)
  , _score(0)
{
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

void GC_Player::Kill(World &world)
{
	if( _vehicle )
		_vehicle->Kill(world); // the reference is released in the OnVehicleKill()
	GC_Service::Kill(world);
}

void GC_Player::SetSkin(std::string skin)
{
	_skin = std::move(skin);
	if( _vehicle )
		_vehicle->SetSkin(std::string("skin/") + _skin);
}

void GC_Player::SetNick(std::string nick)
{
	_nick = std::move(nick);
}

void GC_Player::SetClass(std::string c)
{
	_class = std::move(c);
}

void GC_Player::SetTeam(int team)
{
	_team = team;
}

void GC_Player::SetScore(int score)
{
	_score = score;
}

static GC_SpawnPoint* SelectRespawnPoint(World &world, int team)
{
	std::vector<GC_SpawnPoint*> points;

	GC_SpawnPoint *pBestPoint = nullptr;
	float max_dist = -1;

	FOREACH( world.GetList(LIST_respawns), GC_SpawnPoint, object )
	{
		GC_SpawnPoint *pSpawnPoint = (GC_SpawnPoint*) object;
		if( pSpawnPoint->_team && (pSpawnPoint->_team != team) )
			continue;

		float dist = -1;
		FOREACH( world.GetList(LIST_vehicles), GC_Vehicle, pVeh )
		{
			float d = (pVeh->GetPos() - pSpawnPoint->GetPos()).sqr();
			if( d < dist || dist < 0 ) dist = d;
		}

		if( dist > 0 && dist < 4*CELL_SIZE*CELL_SIZE )
			continue;

		if( dist < 0 || dist > 400*CELL_SIZE*CELL_SIZE )
			points.push_back(pSpawnPoint);

		if( dist > max_dist )
		{
			max_dist = dist;
			pBestPoint = pSpawnPoint;
		}
	}

	if( !points.empty() )
	{
		pBestPoint = points[world.net_rand() % points.size()];
	}

	return pBestPoint;
}

void GC_Player::TimeStep(World &world, float dt)
{
	GC_Service::TimeStep(world, dt);

	if( !GetVehicle() )
	{
		_timeRespawn -= dt;
		if( _timeRespawn <= 0 )
		{
			assert(!GetVehicle());
			_timeRespawn = PLAYER_RESPAWN_DELAY;

			if (GC_SpawnPoint *pBestPoint = SelectRespawnPoint(world, _team))
			{
				world.New<GC_Text_ToolTip>(pBestPoint->GetPos(), _nick, GC_Text::DEFAULT);

				_vehicle = &world.New<GC_Tank_Light>(pBestPoint->GetPos());

				world.New<GC_pu_Shield>(pBestPoint->GetPos()).Attach(world, *_vehicle, true);

				GC_Object* found = world.FindObject(_vehname);
				if( found && _vehicle != found )
				{
//					_logger.Printf(1, "object with name \"%s\" already exists", _vehname.c_str());
				}
				else
				{
					_vehicle->SetName(world, _vehname.c_str());
				}
				_vehicle->SetDirection(pBestPoint->GetDirection());
				_vehicle->SetPlayer(world, this);

				for( auto ls: world.eGC_Player._listeners )
					ls->OnRespawn(*this, *_vehicle);
			}
			else
			{
//				char buf[64];
//				sprintf(buf, _lang.msg_no_respawns_for_team_x.Get().c_str(), _team);
//				_logger.WriteLine(1, buf);
			}
		}
	}
}

void GC_Player::OnVehicleDestroy(World &world)
{
	_numDeaths++;
	_vehicle = nullptr;
	for( auto ls: world.eGC_Player._listeners )
		ls->OnDie(*this);
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
  , _propSkin(      ObjectProperty::TYPE_SKIN,        "skin"    )
  , _propOnDie(     ObjectProperty::TYPE_STRING,      "on_die"  )
  , _propOnRespawn( ObjectProperty::TYPE_STRING,      "on_respawn" )
  , _propVehName(   ObjectProperty::TYPE_STRING,      "vehname" )
{
	_propTeam.SetIntRange(0, MAX_TEAMS);
	_propScore.SetIntRange(INT_MIN, INT_MAX);

	for (unsigned int i = 0; GetVehicleClassName(i); ++i)
		_propClass.AddItem(GetVehicleClassName(i));
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
	return nullptr;
}

void GC_Player::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Player *tmp = static_cast<GC_Player *>(GetObject());

	if( applyToObject )
	{
		tmp->SetTeam(_propTeam.GetIntValue());
		tmp->SetScore(_propScore.GetIntValue());
		tmp->SetNick(std::string(_propNick.GetStringValue()));
		tmp->SetClass(std::string(_propClass.GetListValue(_propClass.GetCurrentIndex())));
		tmp->SetSkin(std::string(_propSkin.GetStringValue()));
		tmp->_scriptOnDie = _propOnDie.GetStringValue();
		tmp->_scriptOnRespawn = _propOnRespawn.GetStringValue();

		auto vehName = _propVehName.GetStringValue();
		if( tmp->GetVehicle() )
		{
			GC_Object* found = world.FindObject(vehName);
			if( found && tmp->GetVehicle() != found )
			{
//				_logger.Printf(1, "WARNING: object with name \"%s\" already exists", name);
			}
			else
			{
				tmp->GetVehicle()->SetName(world, vehName);
				tmp->_vehname = vehName;
			}
		}
		else
		{
			tmp->_vehname = vehName;
		}
	}
	else
	{
		_propOnRespawn.SetStringValue(tmp->_scriptOnRespawn);
		_propOnDie.SetStringValue(tmp->_scriptOnDie);
		_propTeam.SetIntValue(tmp->GetTeam());
		_propScore.SetIntValue(tmp->GetScore());
		_propNick.SetStringValue(std::string(tmp->GetNick()));
		_propVehName.SetStringValue(tmp->_vehname);
		_propSkin.SetStringValue(std::string(tmp->GetSkin()));

		for( size_t i = 0; i < _propClass.GetListSize(); ++i )
		{
			if( tmp->GetClass() == _propClass.GetListValue(i) )
			{
				_propClass.SetCurrentIndex(i);
				break;
			}
		}
	}
}

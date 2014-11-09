#include "Deathmatch.h"
#include "GameEvents.h"
#include "config/Language.h"
#include "gc/GameClasses.h"
#include "gc/Player.h"
#include "gc/Vehicle.h"
#include "gc/World.h"

Deathmatch::Deathmatch(World &world, GameListener &gameListener)
	: _world(world)
	, _gameListener(gameListener)
{
	_world.eGC_RigidBodyStatic.AddListener(*this);
}

Deathmatch::~Deathmatch()
{
	_world.eGC_RigidBodyStatic.RemoveListener(*this);
}

void Deathmatch::OnDestroy(GC_RigidBodyStatic &obj, const DamageDesc &dd)
{
	if (auto vehicle = dynamic_cast<GC_Vehicle*>(&obj))
	{
		char msg[256] = {0};
		char score[8];
		GC_Text::Style style = GC_Text::DEFAULT;
		
		if( dd.from )
		{
			if( dd.from == vehicle->GetOwner() )
			{
				// killed him self
				vehicle->GetOwner()->SetScore(_world, vehicle->GetOwner()->GetScore() - 1);
				style = GC_Text::SCORE_MINUS;
				sprintf(msg, g_lang.msg_player_x_killed_him_self.Get().c_str(), vehicle->GetOwner()->GetNick().c_str());
			}
			else if( vehicle->GetOwner() )
			{
				if( 0 != vehicle->GetOwner()->GetTeam() &&
				   dd.from->GetTeam() == vehicle->GetOwner()->GetTeam() )
				{
					// 'from' killed his friend
					dd.from->SetScore(_world, dd.from->GetScore() - 1);
					style = GC_Text::SCORE_MINUS;
					sprintf(msg, g_lang.msg_player_x_killed_his_friend_x.Get().c_str(),
							dd.from->GetNick().c_str(),
							vehicle->GetOwner()->GetNick().c_str());
				}
				else
				{
					// 'from' killed his enemy
					dd.from->SetScore(_world, dd.from->GetScore() + 1);
					style = GC_Text::SCORE_PLUS;
					sprintf(msg, g_lang.msg_player_x_killed_his_enemy_x.Get().c_str(),
							dd.from->GetNick().c_str(), vehicle->GetOwner()->GetNick().c_str());
				}
			}
			else
			{
				// this tank does not have player service. score up the killer
				dd.from->SetScore(_world, dd.from->GetScore() + 1);
				style = GC_Text::SCORE_PLUS;
			}
			
			if( dd.from->GetVehicle() )
			{
				sprintf(score, "%d", dd.from->GetScore());
				_world.New<GC_Text_ToolTip>(dd.from->GetVehicle()->GetPos(), score, style);
			}
		}
		else if( vehicle->GetOwner() )
		{
			sprintf(msg, g_lang.msg_player_x_died.Get().c_str(), vehicle->GetOwner()->GetNick().c_str());
			vehicle->GetOwner()->SetScore(_world, vehicle->GetOwner()->GetScore() - 1);
			sprintf(score, "%d", vehicle->GetOwner()->GetScore());
			_world.New<GC_Text_ToolTip>(vehicle->GetPos(), score, GC_Text::SCORE_MINUS);
		}
		
		_gameListener.OnGameMessage(msg);
	}
}

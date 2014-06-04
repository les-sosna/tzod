// pickup.cpp

#include "Pickup.h"

#include "GameClasses.h"
#include "indicators.h"
#include "World.h"
#include "Macros.h"
#include "MapFile.h"
#include "particles.h"
#include "Player.h"
#include "SaveFile.h"
#include "script.h"
#include "Sound.h"
#include "vehicle.h"
#include "Weapons.h"

#include <ConsoleBuffer.h>

extern "C"
{
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>
}

UI::ConsoleBuffer& GetConsole();


IMPLEMENT_2LIST_MEMBER(GC_Pickup, LIST_pickups, LIST_timestep);
IMPLEMENT_GRID_MEMBER(GC_Pickup, grid_pickup);

GC_Pickup::GC_Pickup(World &world)
  : GC_2dSprite(world)
  , _label(new GC_HideLabel(world))
  , _radius(25.0)
  , _timeAttached(0)
  , _timeAnimation(0)
  , _timeRespawn(0)
{
    _label->Register(world);

	SetShadow(true);
	SetZ(Z_FREE_ITEM);

	SetAutoSwitch(true);
	SetRespawn(false);
	SetBlinking(false);
}

GC_Pickup::GC_Pickup(FromFile)
  : GC_2dSprite(FromFile())
{
}

GC_Pickup::~GC_Pickup()
{
}

void GC_Pickup::Kill(World &world)
{
	Disappear(world);
	SAFE_KILL(world, _label);
	GC_2dSprite::Kill(world);
}

void GC_Pickup::Serialize(World &world, SaveFile &f)
{
	GC_2dSprite::Serialize(world, f);

	f.Serialize(_pickupCarrier);
	f.Serialize(_timeAttached);
	f.Serialize(_timeAnimation);
	f.Serialize(_timeRespawn);
	f.Serialize(_radius);
	f.Serialize(_scriptOnPickup);
	f.Serialize(_label);
}

GC_Actor* GC_Pickup::FindNewOwner(World &world) const
{
	float r_sq = GetRadius() * GetRadius();

	FOREACH( world.GetList(LIST_vehicles), GC_Vehicle, veh )
	{
		if( (GetPos() - veh->GetPos()).sqr() < r_sq )
		{
			if( GetAutoSwitch() || veh->_state._bState_AllowDrop )
				return veh;
		}
	}

	return NULL;
}

void GC_Pickup::Attach(World &world, GC_Actor *actor)
{
	assert(!_pickupCarrier);
	_pickupCarrier = actor;
	_timeAttached  = 0;
	MoveTo(world, actor->GetPos());
	actor->Subscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_Pickup::OnOwnerMove);
	actor->Subscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Pickup::OnOwnerKill);
	actor->OnPickup(world, this, true);
}

void GC_Pickup::Detach(World &world)
{
	assert(_pickupCarrier);
	SetZ(Z_FREE_ITEM);
	_pickupCarrier->Unsubscribe(NOTIFY_OBJECT_KILL, this, (NOTIFYPROC) &GC_Pickup::OnOwnerKill);
	_pickupCarrier->Unsubscribe(NOTIFY_ACTOR_MOVE, this, (NOTIFYPROC) &GC_Pickup::OnOwnerMove);
	_pickupCarrier->OnPickup(world, this, false);
	_pickupCarrier = NULL;
}

void GC_Pickup::Respawn(World &world)
{
	SetRespawn(false);
	SetVisible(world, true);
	PLAY(SND_puRespawn, GetPos());

	static const TextureCache tex1("particle_1");
	for( int n = 0; n < 50; ++n )
	{
		vec2d a(PI2 * (float) n / 50);
		auto p = new GC_Particle(world, a * 25, tex1, frand(0.5f) + 0.1f);
        p->Register(world);
        p->MoveTo(world, GetPos() + a * 25);
	}
}

void GC_Pickup::Disappear(World &world)
{
	if( GetCarrier() )
		Detach(world);
    SetVisible(world, false);
    PulseNotify(world, NOTIFY_PICKUP_DISAPPEAR);
    _timeAttached = 0;
    if( _label )
        MoveTo(world, _label->GetPos());
}

void GC_Pickup::SetRespawnTime(float respawnTime)
{
	_timeRespawn = respawnTime;
}

float GC_Pickup::GetRespawnTime() const
{
	return _timeRespawn;
}

void GC_Pickup::SetBlinking(bool blink)
{
	SetFlags(GC_FLAG_PICKUP_BLINK, blink);
}

void GC_Pickup::MoveTo(World &world, const vec2d &pos)
{
    if (!CheckFlags(GC_FLAG_PICKUP_KNOWNPOS))
    {
        _label->MoveTo(world, pos);
        SetFlags(GC_FLAG_PICKUP_KNOWNPOS, true);
    }
    GC_2dSprite::MoveTo(world, pos);
}

void GC_Pickup::TimeStepFloat(World &world, float dt)
{
	_timeAnimation += dt;

	if( !GetCarrier() && GetVisible() )
		SetFrame( int((_timeAnimation * ANIMATION_FPS)) % (GetFrameCount()) );

	GC_2dSprite::TimeStepFloat(world, dt);
}

void GC_Pickup::TimeStepFixed(World &world, float dt)
{
	_timeAttached += dt;

	if( !GetCarrier() )
	{
		if( GetVisible() )
		{
			if( GC_Actor *actor = FindNewOwner(world) )
			{
			//	ObjPtr<GC_Object> watch(this);
				// item can be killed inside attach function so create copy
				std::string scriptOnPickup(_scriptOnPickup);
				Attach(world, actor);

				if( !scriptOnPickup.empty() )
				{
					std::stringstream buf;
					buf << "return function(who)";
					buf << scriptOnPickup;
					buf << "\nend";

					if( luaL_loadstring(g_env.L, buf.str().c_str()) )
					{
						GetConsole().Printf(1, "OnPickup: %s", lua_tostring(g_env.L, -1));
						lua_pop(g_env.L, 1); // pop the error message from the stack
					}
					else
					{
						if( lua_pcall(g_env.L, 0, 1, 0) )
						{
							GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
							lua_pop(g_env.L, 1); // pop the error message from the stack
						}
						else
						{
							luaT_pushobject(g_env.L, actor);
							if( lua_pcall(g_env.L, 1, 0, 0) )
							{
								GetConsole().WriteLine(1, lua_tostring(g_env.L, -1));
								lua_pop(g_env.L, 1); // pop the error message from the stack
							}
						}
					}
				}
			}
		}
		else
		{
			if( _timeAttached > _timeRespawn )  // FIXME
				Respawn(world);
		}
	}

	GC_2dSprite::TimeStepFixed(world, dt);
}

void GC_Pickup::Draw(bool editorMode) const
{
	if( !GetBlinking() || fmod(_timeAnimation, 0.16f) > 0.08f || editorMode )
	{
		GC_2dSprite::Draw(editorMode);
	}
}

void GC_Pickup::MapExchange(World &world, MapFile &f)
{
	GC_2dSprite::MapExchange(world, f);
	MAP_EXCHANGE_FLOAT(respawn_time,  _timeRespawn, GetDefaultRespawnTime());
	MAP_EXCHANGE_STRING(on_pickup, _scriptOnPickup, "");
}

void GC_Pickup::OnOwnerMove(World &world, GC_Object *sender, void *param)
{
	assert(GetCarrier());
	assert(GetCarrier() == sender);
	MoveTo(world, GetCarrier()->GetPos());
}

void GC_Pickup::OnOwnerKill(World &world, GC_Object *sender, void *param)
{
	Disappear(world);
}

PropertySet* GC_Pickup::NewPropertySet()
{
	return new MyPropertySet(this);
}

GC_Pickup::MyPropertySet::MyPropertySet(GC_Object *object)
  : BASE(object)
  , _propTimeRespawn(ObjectProperty::TYPE_INTEGER, "respawn_time")
  , _propOnPickup(ObjectProperty::TYPE_STRING, "on_pickup")
{
	_propTimeRespawn.SetIntRange(0, 1000000);
}

int GC_Pickup::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 2;
}

ObjectProperty* GC_Pickup::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	switch( index - BASE::GetCount() )
	{
	case 0: return &_propTimeRespawn; break;
	case 1: return &_propOnPickup;    break;
	}

	assert(false);
	return NULL;
}

void GC_Pickup::MyPropertySet::MyExchange(World &world, bool applyToObject)
{
	BASE::MyExchange(world, applyToObject);

	GC_Pickup *obj = static_cast<GC_Pickup*>(GetObject());
	if( applyToObject )
	{
		obj->_timeRespawn = (float) _propTimeRespawn.GetIntValue() / 1000.0f;
		obj->_scriptOnPickup = _propOnPickup.GetStringValue();
	}
	else
	{
		_propTimeRespawn.SetIntValue(int(obj->_timeRespawn * 1000.0f + 0.5f));
		_propOnPickup.SetStringValue(obj->_scriptOnPickup);
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Health)
{
	ED_ITEM( "pu_health", "obj_health", 4 );
	return true;
}

GC_pu_Health::GC_pu_Health(World &world)
  : GC_Pickup(world)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetTexture("pu_health");
}

GC_pu_Health::GC_pu_Health(FromFile)
  : GC_Pickup(FromFile())
{
}

AIPRIORITY GC_pu_Health::GetPriority(World &world, const GC_Vehicle &veh) const
{
	if( veh.GetHealth() < veh.GetHealthMax() )
		return AIP_HEALTH * (veh.GetHealth() / veh.GetHealthMax());

	return AIP_NOTREQUIRED;
}

void GC_pu_Health::Attach(World &world, GC_Actor *actor)
{
	GC_Pickup::Attach(world, actor);

	static_cast<GC_RigidBodyStatic*>(actor)->SetHealthCur(
		static_cast<GC_RigidBodyStatic*>(actor)->GetHealthMax() );
	PLAY(SND_Pickup, GetPos());

	Disappear(world);
}

GC_Actor* GC_pu_Health::FindNewOwner(World &world) const
{
	GC_Vehicle *vehicle = static_cast<GC_Vehicle *>(GC_Pickup::FindNewOwner(world));

	if( vehicle && !vehicle->_state._bState_AllowDrop &&
		vehicle->GetHealth() >= vehicle->GetHealthMax() )
			return NULL;

	return vehicle;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Mine)
{
	ED_ITEM( "pu_mine", "obj_mine", 5 );
	return true;
}

GC_pu_Mine::GC_pu_Mine(World &world)
  : GC_Pickup(world)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetTexture("item_mine");
	SetShadow(false);
}

GC_pu_Mine::GC_pu_Mine(FromFile)
  : GC_Pickup(FromFile())
{
}

AIPRIORITY GC_pu_Mine::GetPriority(World &world, const GC_Vehicle &veh) const
{
	return AIP_NOTREQUIRED;
}

void GC_pu_Mine::Attach(World &world, GC_Actor *actor)
{
//	GC_Pickup::Attach(actor);

//	assert(dynamic_cast<GC_RigidBodyStatic*>(actor));
	(new GC_Boom_Standard(world, GetPos(), NULL))->Register(world);
	Kill(world);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Shield)
{
	ED_ITEM( "pu_shield", "obj_shield", 4 );
	return true;
}

GC_pu_Shield::GC_pu_Shield(World &world)
  : GC_Pickup(world)
{
	SetRespawnTime( GetDefaultRespawnTime() );
	SetTexture("pu_inv");
	SetAutoSwitch(true);

	_timeHit = 0;
}

GC_pu_Shield::GC_pu_Shield(FromFile)
  : GC_Pickup(FromFile())
{
}

AIPRIORITY GC_pu_Shield::GetPriority(World &world, const GC_Vehicle &veh) const
{
	return AIP_SHIELD;
}

void GC_pu_Shield::Attach(World &world, GC_Actor *actor)
{
	//if( GC_Object *p = actor->GetSubscriber(GetType()) )
	//{
	//	assert(dynamic_cast<GC_pu_Shield*>(p));
	//	static_cast<GC_Pickup*>(p)->Disappear(world);
	//}

	GC_Pickup::Attach(world, actor);

	GetCarrier()->Subscribe(NOTIFY_DAMAGE_FILTER, this, (NOTIFYPROC) &GC_pu_Shield::OnOwnerDamage);

	SetZ(Z_PARTICLE);
	SetTexture("shield");
	SetShadow(false);

	PLAY(SND_Inv, GetPos());
}

void GC_pu_Shield::Detach(World &world)
{
	GetCarrier()->Unsubscribe(NOTIFY_DAMAGE_FILTER, this, (NOTIFYPROC) &GC_pu_Shield::OnOwnerDamage);

	SetTexture("pu_inv");
	SetShadow(true);
	SetBlinking(false);
	SetOpacity(1);

	GC_Pickup::Detach(world);
}

void GC_pu_Shield::TimeStepFixed(World &world, float dt)
{
	GC_Pickup::TimeStepFixed(world, dt);

	if( GetCarrier() )
	{
		if( GetTimeAttached() + 2.0f > PROTECT_TIME )
		{
			if( !GetBlinking() )
			{
				PLAY(SND_InvEnd, GetPos());
				SetBlinking(true);
			}
			SetOpacity( (PROTECT_TIME - GetTimeAttached()) / 2.0f );

			if( GetTimeAttached() > PROTECT_TIME )
			{
				Disappear(world);
			}
		}
	}
}

void GC_pu_Shield::TimeStepFloat(World &world, float dt)
{
	GC_Pickup::TimeStepFloat(world, dt);
	if( GetCarrier() )
	{
		_timeHit = std::max(.0f, _timeHit - dt);
		SetFrame( int((GetTimeAnimation() * ANIMATION_FPS)) % (GetFrameCount()) );
	}
}

void GC_pu_Shield::OnOwnerDamage(World &world, GC_Object *sender, void *param)
{
	static TextureCache tex("particle_3");

	DamageDesc *pdd = reinterpret_cast<DamageDesc*>(param);
	assert(NULL != pdd);
	if( pdd->damage > 5 || 0 == rand() % 4 || 0 == _timeHit )
	{
		const vec2d &pos = static_cast<GC_Actor*>(sender)->GetPos();

		_timeHit = 0.2f;

		PLAY(rand() % 2 ? SND_InvHit1 : SND_InvHit2, pos);
		vec2d dir = (pdd->hit - pos).Normalize();
		vec2d p   = vec2d(dir.y, -dir.x);
		vec2d v   = ((GC_Vehicle *) sender)->_lv;
		for( int i = 0; i < 7; i++ )
		{
			auto p1 = new GC_Particle(world, v, tex, frand(0.4f)+0.1f);
            p1->Register(world);
            p1->MoveTo(world, pos + dir * 26.0f + p * (float) (i<<1));
			auto p2 = new GC_Particle(world, v, tex, frand(0.4f)+0.1f);
            p2->Register(world);
            p2->MoveTo(world, pos + dir * 26.0f - p * (float) (i<<1));
		}
	}
	pdd->damage *= 0.25;
}

void GC_pu_Shield::Serialize(World &world, SaveFile &f)
{
	GC_Pickup::Serialize(world, f);
	f.Serialize(_timeHit);
}

///////////////////////////////////////////////////////////////////////////////

static const float SHOCK_TIMEOUT = 1.5f;

IMPLEMENT_SELF_REGISTRATION(GC_pu_Shock)
{
	ED_ITEM( "pu_shock", "obj_shock", 4 );
	return true;
}

IMPLEMENT_1LIST_MEMBER(GC_pu_Shock, LIST_gsprites);

GC_pu_Shock::GC_pu_Shock(World &world)
  : GC_Pickup(world)
  , _targetPosPredicted(0, 0)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetAutoSwitch(false);
	SetTexture("pu_shock");
}

GC_pu_Shock::GC_pu_Shock(FromFile)
  : GC_Pickup(FromFile())
{
}

GC_pu_Shock::~GC_pu_Shock()
{
}

void GC_pu_Shock::Kill(World &world)
{
    SAFE_KILL(world, _light);
    GC_Pickup::Kill(world);
}

void GC_pu_Shock::Serialize(World &world, SaveFile &f)
{
	GC_Pickup::Serialize(world, f);
	f.Serialize(_light);
	f.Serialize(_targetPosPredicted);
}

AIPRIORITY GC_pu_Shock::GetPriority(World &world, const GC_Vehicle &veh) const
{
	GC_Vehicle *tmp = FindNearVehicle(world, &veh);
	if( !tmp ) return AIP_NOTREQUIRED;

	if( tmp->GetOwner()->GetTeam() == veh.GetOwner()->GetTeam() && 0 != tmp->GetOwner()->GetTeam() )
	{
		return AIP_NOTREQUIRED;
	}

	return AIP_SHOCK;
}

void GC_pu_Shock::Attach(World &world, GC_Actor* actor)
{
	assert(dynamic_cast<GC_RigidBodyStatic*>(actor));

	GC_Pickup::Attach(world, actor);
	PLAY(SND_ShockActivate, GetPos());
	SetTexture("explosion_g");
}

void GC_pu_Shock::Detach(World &world)
{
	GC_Pickup::Detach(world);
	SetTexture("pu_shock");
	SetGridSet(true);
	SAFE_KILL(world, _light);
}

GC_Vehicle* GC_pu_Shock::FindNearVehicle(World &world, const GC_RigidBodyStatic *ignore) const
{
	//
	// find the nearest enemy
	//

	float min_dist = AI_MAX_SIGHT * CELL_SIZE;
	float dist;

	GC_Vehicle *pNearTarget = NULL;
	FOREACH( world.GetList(LIST_vehicles), GC_Vehicle, pTargetObj )
	{
		if( pTargetObj != ignore )
		{
			// distance to the object
			dist = (GetPos() - pTargetObj->GetPos()).len();

			if( dist < min_dist )
			{
				GC_RigidBodyStatic *pObstacle = world.TraceNearest(world.grid_rigid_s,
					static_cast<GC_RigidBodyStatic*>(GetCarrier()),
					GetPos(), pTargetObj->GetPos() - GetPos());

				if( pObstacle == pTargetObj )
				{
					pNearTarget = pTargetObj;
					min_dist = dist;
				}
			}
		}
	}

	return pNearTarget;
}

void GC_pu_Shock::TimeStepFixed(World &world, float dt)
{
	GC_Pickup::TimeStepFixed(world, dt);

	if( GetCarrier() )
	{
		if( GetGridSet() )
		{
			if( GetTimeAttached() >= SHOCK_TIMEOUT )
			{
				GC_RigidBodyStatic *carrier = static_cast<GC_RigidBodyStatic *>(GetCarrier());
				if( GC_Vehicle *pNearTarget = FindNearVehicle(world, carrier) )
				{
					SetGridSet(false);
					SetZ(Z_FREE_ITEM);

					_targetPosPredicted = pNearTarget->GetPos();

					_light = new GC_Light(world, GC_Light::LIGHT_DIRECT);
                    _light->Register(world);
					_light->MoveTo(world, GetPos());
					_light->SetRadius(100);

					vec2d tmp = _targetPosPredicted - GetPos();
					_light->SetLength(tmp.len());
					_light->SetLightDirection(tmp.Normalize());

					pNearTarget->TakeDamage(world, 1000, pNearTarget->GetPos(), carrier->GetOwner());
				}
				else
				{
					carrier->TakeDamage(world, 1000, GetCarrier()->GetPos(), carrier->GetOwner());
					Disappear(world);
				}
			}
		}
		else
		{
			float a = (GetTimeAttached() - SHOCK_TIMEOUT) * 5.0f;
			if( a > 1 )
			{
				Disappear(world);
			}
			else
			{
				_light->SetIntensity(1.0f - powf(a, 6));
			}
		}
	}
}

void GC_pu_Shock::Draw(bool editorMode) const
{
	if( GetGridSet() )
	{
		GC_Pickup::Draw(editorMode);
	}
	else
	{
		static TextureCache t("lightning");
		SpriteColor c;
		c.r = c.g = c.b = c.a = int((1.0f - ((GetTimeAttached() - SHOCK_TIMEOUT) * 5.0f)) * 255.0f);
		const vec2d &pos = GetPos();
		g_texman->DrawLine(t.GetTexture(), c, pos.x, pos.y, _targetPosPredicted.x, _targetPosPredicted.y, frand(1));
	}
}

///////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Booster)
{
	ED_ITEM( "pu_booster", "obj_booster", 4 );
	return true;
}

GC_pu_Booster::GC_pu_Booster(World &world)
  : GC_Pickup(world)
{
	SetRespawnTime(GetDefaultRespawnTime());
	SetTexture("pu_booster");
}

GC_pu_Booster::GC_pu_Booster(FromFile)
  : GC_Pickup(FromFile())
{
}

GC_pu_Booster::~GC_pu_Booster()
{
}

void GC_pu_Booster::Serialize(World &world, SaveFile &f)
{
	GC_Pickup::Serialize(world, f);
	f.Serialize(_sound);
}

AIPRIORITY GC_pu_Booster::GetPriority(World &world, const GC_Vehicle &veh) const
{
	if( !veh.GetWeapon() )
	{
		return AIP_NOTREQUIRED;
	}

	return veh.GetWeapon()->GetAdvanced() ? AIP_BOOSTER_HAVE : AIP_BOOSTER;
}

void GC_pu_Booster::Attach(World &world, GC_Actor* actor)
{
	GC_Weapon *w = dynamic_cast<GC_Weapon*>(actor);

	if( NULL == w ) // disappear if actor is not a weapon.
	{
		PLAY(SND_B_End, actor->GetPos());
		Disappear(world);
		return;
	}

	GC_Pickup::Attach(world, w);

	if( w->GetAdvanced() )
	{
		// find existing booster
		FOREACH( world.GetList(LIST_pickups), GC_Pickup, pickup )
		{
			if( pickup->GetType() == GetType() && this != pickup )
			{
				assert(dynamic_cast<GC_pu_Booster*>(pickup));
				if( static_cast<GC_pu_Booster*>(pickup)->GetCarrier() == w )
				{
					pickup->Disappear(world); // detach previous booster
					break;
				}
			}
		}
	}

	w->SetAdvanced(world, true);
	w->Subscribe(NOTIFY_PICKUP_DISAPPEAR, this, (NOTIFYPROC) &GC_pu_Booster::OnWeaponDisappear);

	PLAY(SND_B_Start, GetPos());
	assert(NULL == _sound);
	_sound = new GC_Sound_link(world, SND_B_Loop, this);
    _sound->Register(world);
    _sound->SetMode(world, SMODE_LOOP);

	SetTexture("booster");
	SetShadow(false);
}

void GC_pu_Booster::Detach(World &world)
{
	assert(dynamic_cast<GC_Weapon*>(GetCarrier()));
	static_cast<GC_Weapon*>(GetCarrier())->SetAdvanced(world, false);
	GetCarrier()->Unsubscribe(NOTIFY_PICKUP_DISAPPEAR, this, (NOTIFYPROC) &GC_pu_Booster::OnWeaponDisappear);
	SetTexture("pu_booster");
	SetShadow(true);
	SAFE_KILL(world, _sound);
	GC_Pickup::Detach(world);
}

GC_Actor* GC_pu_Booster::FindNewOwner(World &world) const
{
	GC_Vehicle *veh = static_cast<GC_Vehicle *>(GC_Pickup::FindNewOwner(world));
	if( veh && !veh->_state._bState_AllowDrop && !veh->GetWeapon() )
		return NULL;
	return (veh && veh->GetWeapon()) ? veh->GetWeapon() : static_cast<GC_Actor *>(veh);
}

void GC_pu_Booster::TimeStepFloat(World &world, float dt)
{
	GC_Pickup::TimeStepFloat(world, dt);
	if( GetCarrier() )
	{
		SetDirection(vec2d(GetTimeAnimation() * 50));
	}
}

void GC_pu_Booster::TimeStepFixed(World &world, float dt)
{
	GC_Pickup::TimeStepFixed(world, dt);
	if( GetCarrier() )
	{
		if( GetTimeAttached() > BOOSTER_TIME )
		{
			PLAY(SND_B_End, GetPos());
			Disappear(world);
		}
	}
}

void GC_pu_Booster::OnWeaponDisappear(World &world, GC_Object *sender, void *param)
{
	assert(GetCarrier());
	Disappear(world);
}

///////////////////////////////////////////////////////////////////////////////
// end of file

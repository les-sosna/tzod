// pickup.cpp

#include "stdafx.h"
#include "pickup.h"
#include "constants.h"
#include "macros.h"
#include "script.h"
#include "Level.h"
#include "options.h"
#include "functions.h"

#include "fs/SaveFile.h"
#include "fs/MapFile.h"

#include "ai.h"

#include "gameclasses.h"
#include "indicators.h"
#include "vehicle.h"
#include "player.h"
#include "sound.h"
#include "particles.h"
#include "projectiles.h"

#include "Weapons.h"

////////////////////////////////////////////////////////

GC_Item::GC_Item(float x, float y)
{
	MoveTo(vec2d(x, y));

	_label  = new GC_HideLabel(_pos.x, _pos.y);
	_radius = 25.0;
}

GC_Item::GC_Item(FromFile) : GC_2dSprite(FromFile())
{
}

void GC_Item::Kill()
{
	SAFE_KILL(_label);
	GC_2dSprite::Kill();
}

void GC_Item::Serialize(SaveFile &f)
{
	GC_2dSprite::Serialize(f);
	////////////////////////////////////////
	f.Serialize(_radius);
	f.Serialize(_label);
}


GC_Vehicle* GC_PickUp::CheckPickUp()
{
	float r_sq = getRadius() * getRadius();

	FOREACH( vehicles, GC_Vehicle, pVehicle )
	{
		if( !pVehicle->IsKilled() )
		{
			if( r_sq >  (_pos.x - pVehicle->_pos.x) * (_pos.x - pVehicle->_pos.x) +
					(_pos.y - pVehicle->_pos.y) * (_pos.y - pVehicle->_pos.y) )
			{
				if( !_bMostBeAllowed || pVehicle->_state._bState_AllowDrop )
					return pVehicle;
			}
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////

GC_PickUp::GC_PickUp(float x, float y)
    : GC_Item(x, y), _memberOf(g_level->pickups, this)
{
	AddContext(&g_level->grid_pickup);

	_bMostBeAllowed = false;
	_time_respawn   = 0;
	_time_animation = 0;
	_time           = 0;
	_bRespawn       = true;
	_attached      = false;
	_blink          = false;

	SetShadow(true);
	SetEvents(GC_FLAG_OBJECT_EVENTS_TS_FLOATING | GC_FLAG_OBJECT_EVENTS_TS_FIXED);
	SetZ(Z_FREE_ITEM);


	MoveTo(vec2d(x, y));
}

GC_PickUp::GC_PickUp(FromFile)
  : GC_Item(FromFile()), _memberOf(g_level->pickups, this)
{
}

void GC_PickUp::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_Item::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_attached);
	f.Serialize(_blink);
	f.Serialize(_bMostBeAllowed);
	f.Serialize(_bRespawn);
	f.Serialize(_time);
	f.Serialize(_time_animation);
	f.Serialize(_time_respawn);
	/////////////////////////////////////
	f.Serialize(_ancObject);
	/////////////////////////////////////
	if( !IsKilled() && f.loading() )
		AddContext(&g_level->grid_pickup);
}

void GC_PickUp::Kill()
{
	_ancObject = NULL;
	GC_Item::Kill();
}

void GC_PickUp::GiveIt(GC_Vehicle* pVehicle)
{
	_time = 0;

	if( _bRespawn )
	{
		GC_PickUp *object = SetRespawn();
		object->Show(false);
		object->_time_respawn = _time_respawn;
		object->SetAnchor(this);
	}

	MoveTo(pVehicle->_pos);
}

void GC_PickUp::Respawn()
{
	Show(true);
	PLAY(SND_puRespawn, _pos);

	static const TextureCache tex1("particle_1");
	for( int n = 0; n < 50; ++n )
	{
		vec2d a(PI2 * (float) n / 50);
		new GC_Particle(_pos + a * 25, a * 25, tex1, frand(0.5f) + 0.1f);
	}
}

void GC_PickUp::SetAnchor(GC_Object *object)
{
	_ancObject = object;
}

void GC_PickUp::SetBlinking(bool blink)
{
	_ASSERT(CheckFlags(GC_FLAG_OBJECT_EVENTS_TS_FLOATING));
	_blink = blink;
}

void GC_PickUp::TimeStepFloat(float dt)
{
	_time_animation += dt;

	if( !_attached && IsVisible() )
		SetFrame( int((_time_animation * ANIMATION_FPS)) % (GetFrameCount()) );

	GC_Item::TimeStepFloat(dt);
}

void GC_PickUp::TimeStepFixed(float dt)
{
	_time += dt;

	if( !_attached )
	{
		if( IsVisible() )
		{
			GC_Vehicle *pVehicle = CheckPickUp();
			if( pVehicle ) GiveIt(pVehicle);
		}
		else
		{
			if( _ancObject )
			{
				if( _ancObject->IsKilled() )
					_ancObject = NULL;
				else
					_time -= dt;
			}

			if( _time > _time_respawn )
				Respawn();
		}
	}

	GC_Item::TimeStepFixed(dt);
}

void GC_PickUp::Draw()
{
	if( !_blink || fmodf(_time_animation, 0.16f) > 0.08f || g_level->_modeEditor )
		GC_Item::Draw();
}

void GC_PickUp::mapExchange(MapFile &f)
{
	GC_Item::mapExchange(f);
	MAP_EXCHANGE_FLOAT(respawn_time,  _time_respawn, GetDefaultRespawnTime());
}

SafePtr<PropertySet> GC_PickUp::GetProperties()
{
	return new MyPropertySet(this);
}

GC_PickUp::MyPropertySet::MyPropertySet(GC_Object *object)
: BASE(object)
, _propTimeRespawn(ObjectProperty::TYPE_INTEGER, "Respawn Time")
{
	_propTimeRespawn.SetRange(0, 1000000);
	//-----------------------------------------
	Exchange(false);
}

int GC_PickUp::MyPropertySet::GetCount() const
{
	return BASE::GetCount() + 1;
}

ObjectProperty* GC_PickUp::MyPropertySet::GetProperty(int index)
{
	if( index < BASE::GetCount() )
		return BASE::GetProperty(index);

	if( BASE::GetCount() + 0 == index )
		return &_propTimeRespawn;

	_ASSERT(FALSE);
	return NULL;
}

void GC_PickUp::MyPropertySet::Exchange(bool bApply)
{
	BASE::Exchange(bApply);

	GC_PickUp *obj = static_cast<GC_PickUp*>(GetObject());
	if( bApply )
	{
		obj->_time_respawn = (float) _propTimeRespawn.GetValueInt() / 1000.0f;
	}
	else
	{
		_propTimeRespawn.SetValueInt(int(obj->_time_respawn * 1000.0f + 0.5f));
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Health)
{
	ED_ITEM( "pu_health", "Предмет:\tАптека" );
	return true;
}

GC_pu_Health::GC_pu_Health(float x, float y) : GC_PickUp(x, y)
{
	_time_respawn = GetDefaultRespawnTime();
	SetTexture("pu_health");
}

GC_pu_Health::GC_pu_Health(FromFile) : GC_PickUp(FromFile())
{
}

AIPRIORITY GC_pu_Health::CheckUseful(GC_Vehicle *pVehicle)
{
	if( pVehicle->GetHealth() < pVehicle->GetHealthMax() )
		return AIP_HEALTH * (pVehicle->GetHealth() / pVehicle->GetHealthMax());

	return AIP_NOTREQUIRED;
}

void GC_pu_Health::GiveIt(GC_Vehicle* pVehicle)
{
	GC_PickUp::GiveIt(pVehicle);

	pVehicle->SetHealthCur(pVehicle->GetHealthMax());
	PLAY(SND_PickUp, _pos);

	Kill();
}

GC_PickUp* GC_pu_Health::SetRespawn()
{
	return new GC_pu_Health(_pos.x, _pos.y);
}

GC_Vehicle* GC_pu_Health::CheckPickUp()
{
	GC_Vehicle *vehicle = GC_PickUp::CheckPickUp();

	if( vehicle && !vehicle->_state._bState_AllowDrop &&
		vehicle->GetHealth() >= vehicle->GetHealthMax() )
			return NULL;

	return vehicle;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Mine)
{
	ED_ITEM( "pu_mine", "Предмет:\tПротивотанковая мина" );
	return true;
}

GC_pu_Mine::GC_pu_Mine(float x, float y) : GC_PickUp(x, y)
{
	_time_respawn = GetDefaultRespawnTime();
	SetTexture("item_mine");
	SetShadow(false);
}

GC_pu_Mine::GC_pu_Mine(FromFile) : GC_PickUp(FromFile())
{
}

AIPRIORITY GC_pu_Mine::CheckUseful(GC_Vehicle *pVehicle)
{
	return AIP_NOTREQUIRED;
}

void GC_pu_Mine::GiveIt(GC_Vehicle* pVehicle)
{
	GC_PickUp::GiveIt(pVehicle);
	new GC_Boom_Standard(_pos, pVehicle);
	Kill();
}

GC_PickUp* GC_pu_Mine::SetRespawn()
{
	return new GC_pu_Mine(_pos.x, _pos.y);
}

GC_Vehicle* GC_pu_Mine::CheckPickUp()
{
	GC_Vehicle *pVehicle = GC_PickUp::CheckPickUp();
	return pVehicle;
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Invulnerablity)
{
	ED_ITEM( "pu_shield", "Предмет:\tНеуязвимость" );
	return true;
}

GC_pu_Invulnerablity::GC_pu_Invulnerablity(float x, float y) : GC_PickUp(x, y)
{
	_time_hit       = 0;
	_time_respawn   = GetDefaultRespawnTime();
	_bMostBeAllowed = false;

	SetTexture("pu_inv");
}

GC_pu_Invulnerablity::GC_pu_Invulnerablity(FromFile) : GC_PickUp(FromFile())
{
}

AIPRIORITY GC_pu_Invulnerablity::CheckUseful(GC_Vehicle *pVehicle)
{
	return AIP_INVULN;
}

void GC_pu_Invulnerablity::GiveIt(GC_Vehicle* pVehicle)
{
	GC_PickUp::GiveIt(pVehicle);

	PLAY(SND_Inv, _pos);

	GC_pu_Invulnerablity *pOther = (GC_pu_Invulnerablity *) pVehicle->GetSubscriber(GetType());
	if( pOther ) pOther->Kill();

	pVehicle->Subscribe(NOTIFY_DAMAGE_FILTER, this,
		(NOTIFYPROC) &GC_pu_Invulnerablity::OnProprietorDamage, false);
	pVehicle->Subscribe(NOTIFY_OBJECT_MOVE, this,
		(NOTIFYPROC) &GC_pu_Invulnerablity::OnProprietorMove, false);

	_attached = true;
	SetZ(Z_PARTICLE);
	SetTexture("shield");
	SetShadow(false);
}

GC_PickUp* GC_pu_Invulnerablity::SetRespawn()
{
	return new GC_pu_Invulnerablity(_pos.x, _pos.y);
}

void GC_pu_Invulnerablity::TimeStepFixed(float dt)
{
	GC_PickUp::TimeStepFixed(dt);

	if( _attached )
	{
		if( _time + 2.0f > PROTECT_TIME )
		{
			if( !GetBlinking() )
			{
				PLAY(SND_InvEnd, _pos);
				SetBlinking(true);
			}
			SetOpacity( (PROTECT_TIME - _time) / 2.0f );
		}
		else
		{
			SetBlinking(false);
		}

		if( _time > PROTECT_TIME )
		{
			Kill();
			return;
		}
	}
}

void GC_pu_Invulnerablity::TimeStepFloat(float dt)
{
	GC_PickUp::TimeStepFloat(dt);
	if( _attached )
	{
		_time_hit = __max(0, _time_hit - dt);
		SetFrame( int((_time_animation * ANIMATION_FPS)) % (GetFrameCount()) );
	}
}

void GC_pu_Invulnerablity::OnProprietorDamage(GC_Object *sender, void *param)
{
	static TextureCache tex("particle_3");

	LPDAMAGEDESC pdd = (LPDAMAGEDESC) param;
	_ASSERT(NULL != pdd);
	if( pdd->damage > 5 || 0 == rand() % 4 || 0 == _time_hit )
	{
		_time_hit = 0.2f;

		PLAY(rand() % 2 ? SND_InvHit1 : SND_InvHit2, sender->_pos);
		vec2d dir = (pdd->hit - sender->_pos).Normalize();
		vec2d p   = vec2d(dir.y, -dir.x);
		vec2d v   = ((GC_Vehicle *) sender)->_lv;
		for( int i = 0; i < 7; i++ )
		{
			new GC_Particle(sender->_pos + dir * 26.0f + p * (float) (i<<1), v, tex, frand(0.4f)+0.1f);
			new GC_Particle(sender->_pos + dir * 26.0f - p * (float) (i<<1), v, tex, frand(0.4f)+0.1f);
		}
	}
	pdd->damage = 0;
}

void GC_pu_Invulnerablity::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_PickUp::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_time_hit);
}

void GC_pu_Invulnerablity::OnProprietorMove(GC_Object *sender, void *param)
{
	MoveTo(sender->_pos);
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Shock)
{
	ED_ITEM( "pu_shock", "Предмет:\tЭлектрошок" );
	return true;
}

GC_pu_Shock::GC_pu_Shock(float x, float y) : GC_PickUp(x, y)
{
	_time_respawn = GetDefaultRespawnTime();
	_time_wait = 1.5f;

	_bMostBeAllowed = true;

	SetTexture("pu_shock");
}

GC_pu_Shock::GC_pu_Shock(FromFile) : GC_PickUp(FromFile())
{
}

GC_pu_Shock::~GC_pu_Shock()
{
}

void GC_pu_Shock::Kill()
{
	SAFE_KILL(_light);
	SAFE_KILL(_effect);
	_vehicle = NULL;
	GC_PickUp::Kill();
}

void GC_pu_Shock::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_PickUp::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_time_wait);
	/////////////////////////////////////
	f.Serialize(_effect);
	f.Serialize(_light);
	f.Serialize(_vehicle);
}

AIPRIORITY GC_pu_Shock::CheckUseful(GC_Vehicle *pVehicle)
{
	GC_Vehicle *tmp = FindNearVehicle(pVehicle);
	if( !tmp ) return AIP_NOTREQUIRED;

	if( tmp->GetPlayer()->GetTeam() == pVehicle->GetPlayer()->GetTeam() &&
		0 != tmp->GetPlayer()->GetTeam() )
	{
		return AIP_NOTREQUIRED;
	}

	return AIP_SHOCK;
}

void GC_pu_Shock::GiveIt(GC_Vehicle* pVehicle)
{
	GC_PickUp::GiveIt(pVehicle);

	_vehicle = pVehicle;

	_attached = true;
	Show(false);

	PLAY(SND_ShockActivate, _pos);
}

GC_PickUp* GC_pu_Shock::SetRespawn()
{
	return new GC_pu_Shock(_pos.x, _pos.y);
}

GC_Vehicle* GC_pu_Shock::FindNearVehicle(GC_Vehicle *pIgnore)
{
	//
	// находим ближайшего врага
	//

	float min_dist = AI_MAX_SIGHT * CELL_SIZE;
	float dist;

	GC_Vehicle *pNearTarget = NULL;
	FOREACH( vehicles, GC_Vehicle, pTargetObj )
	{
		if( !pTargetObj->IsKilled() && pTargetObj != pIgnore )
		{
			// расстояние до объекта
			dist = (_pos - pTargetObj->_pos).Length();

			if( dist < min_dist )
			{
				GC_RigidBodyStatic *pObstacle = g_level->agTrace(
					g_level->grid_rigid_s, GetRawPtr(_vehicle), _pos, pTargetObj->_pos - _pos);

				if( pObstacle == pTargetObj )
				{
					// трассировка уперлась в целевой объект. запомним его.
					pNearTarget = pTargetObj;
					min_dist = dist;
				}
			}
		}
	}

	return pNearTarget;
}

void GC_pu_Shock::TimeStepFixed(float dt)
{
	GC_PickUp::TimeStepFixed(dt);

	if( _attached )
	{
		if( !_effect )
		{
			if( _time > _time_wait )
			{

				if( _vehicle->IsKilled() )
				{
					Kill();
					return;
				}

				MoveTo(_vehicle->_pos);
				GC_Vehicle *pNearTarget = FindNearVehicle(GetRawPtr(_vehicle));

				if( pNearTarget )
				{
					_effect = new GC_Line(_pos, pNearTarget->_pos, "lighting");
					_effect->SetPhase(frand(1));
					_effect->SetZ(Z_FREE_ITEM);

					_light = new GC_Light(GC_Light::LIGHT_DIRECT);
					_light->MoveTo(_pos);
					_light->SetRadius(100);
					_light->SetLength((pNearTarget->_pos - _pos).Length());
					_light->SetAngle((pNearTarget->_pos - _pos).Angle());

					_time = 0;

					pNearTarget->TakeDamage(1000.0, pNearTarget->_pos, GetRawPtr(_vehicle));
				}
				else
				{
					_vehicle->TakeDamage(1000.0, _vehicle->_pos, GetRawPtr(_vehicle));
					Kill();
				}
			}
		}
		else
		{
			float a = _time / 0.2f;
			if( a > 1 )
			{
				SAFE_KILL(_effect);
				Kill();
			}
			else
			{
				_effect->SetOpacity(1.0f - a);
			}
		}
	}
}

/////////////////////////////////////////////////////////////

IMPLEMENT_SELF_REGISTRATION(GC_pu_Booster)
{
	ED_ITEM( "pu_booster", "Предмет:\tБустер оружия" );
	return true;
}

GC_pu_Booster::GC_pu_Booster(float x, float y) : GC_PickUp(x, y)
{
	_time_respawn = GetDefaultRespawnTime();
	SetTexture("pu_booster");
}

GC_pu_Booster::GC_pu_Booster(FromFile) : GC_PickUp(FromFile())
{
}

GC_pu_Booster::~GC_pu_Booster()
{
}

void GC_pu_Booster::Kill()
{
	if( _weapon )
	{
		_weapon->SetAdvanced(false);
		_weapon = NULL;
	}

	GC_PickUp::Kill();
}

void GC_pu_Booster::Serialize(SaveFile &f)
{	/////////////////////////////////////
	GC_PickUp::Serialize(f);
	/////////////////////////////////////
	f.Serialize(_weapon);
}

AIPRIORITY GC_pu_Booster::CheckUseful(GC_Vehicle *pVehicle)
{
	if( !pVehicle->GetWeapon() )
	{
		return AIP_NOTREQUIRED;
	}

	return pVehicle->GetWeapon()->GetAdvanced() ? AIP_BOOSTER_HAVE : AIP_BOOSTER;
}

void GC_pu_Booster::GiveIt(GC_Vehicle* pVehicle)		//return true  -  respawn
{
	GC_PickUp::GiveIt(pVehicle);

	if( !pVehicle->GetWeapon() )
	{
		PLAY(SND_B_End, _pos);
		Kill();
		return;
	}

	_weapon = pVehicle->GetWeapon();
	_attached = true;

	if( _weapon->GetAdvanced() )
	{
		FOREACH( pickups, GC_PickUp, ppu )
		{
			if( ppu == this || ppu->GetType() != this_type ) continue;

			if( ((GC_pu_Booster *) ppu)->_weapon == _weapon )
			{
				ppu->Kill();
				break;
			}
		}
	}

	_weapon->SetAdvanced(true);
	MoveTo(_weapon->_pos);

	PLAY(SND_B_Start, _pos);
	new GC_Sound_link(SND_B_Loop, SMODE_LOOP, this);

	SetTexture("booster");
	SetShadow(false);
}

GC_PickUp* GC_pu_Booster::SetRespawn()
{
	return new GC_pu_Booster(_pos.x, _pos.y);
}

GC_Vehicle* GC_pu_Booster::CheckPickUp()
{
	GC_Vehicle *pVehicle = GC_PickUp::CheckPickUp();

	if( pVehicle )
		if( !pVehicle->_state._bState_AllowDrop && !pVehicle->GetWeapon() )
			return NULL;

	return pVehicle;
}

void GC_pu_Booster::TimeStepFloat(float dt)
{
	GC_PickUp::TimeStepFloat(dt);

	if( _attached )
	{
		SetRotation(_time_animation * 50);
	}
}

void GC_pu_Booster::TimeStepFixed(float dt)
{
	GC_PickUp::TimeStepFixed(dt);

	if( _weapon )
	{
		if( _weapon->IsKilled() )
		{
			Kill();
			PLAY(SND_B_End, _pos);
		}
		else
		{
			MoveTo(_weapon->_pos);
		}
	}

	if( _attached && _time > BOOSTER_TIME && !IsKilled() )
	{
		Kill();
		PLAY(SND_B_End, _pos);
	}
}

///////////////////////////////////////////////////////////////////////////////
// end of file

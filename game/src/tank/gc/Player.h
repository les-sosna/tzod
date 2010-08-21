// Player.h

#pragma once

#include "Service.h"
#include "network/ControlPacket.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

struct VehicleState;
class GC_Vehicle;

///////////////////////////////////////////////////////////////////////////////

class GC_Player : public GC_Service
{
	MemberOfGlobalList<LIST_players> _memberOf;

	float     _timeRespawn;

	int       _team;
	int       _score;
	string_t  _nick;
	string_t  _class;
	string_t  _skin;
	string_t  _vehname;
	string_t  _scriptOnDie;
	string_t  _scriptOnRespawn;

	SafePtr<GC_Vehicle> _vehicle;

protected:
	class MyPropertySet : public GC_Service::MyPropertySet
	{
		typedef GC_Service::MyPropertySet BASE;

		ObjectProperty _propTeam;
		ObjectProperty _propScore;
		ObjectProperty _propNick;
		ObjectProperty _propClass;
		ObjectProperty _propSkin;
		ObjectProperty _propOnDie;
		ObjectProperty _propOnRespawn;
		ObjectProperty _propVehName;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();

protected:
	virtual void OnRespawn();
	virtual void OnDie();

public:
	void SetVehicle(const SafePtr<GC_Vehicle> &car);
	GC_Vehicle* GetVehicle() const { return GetRawPtr(_vehicle); }

	void SetSkin(const string_t &skin);
	const string_t& GetSkin() const { return _skin; }

	void SetNick(const string_t &nick);
	const string_t& GetNick() const { return _nick; }

	void SetClass(const string_t &c);
	const string_t& GetClass() const { return _class; }

	void SetTeam(int team);
	int GetTeam() const { return _team; }

	void SetScore(int score);
	int GetScore() const { return _score; }

public:
	GC_Player();
	GC_Player(FromFile);
	virtual ~GC_Player();
	virtual void Kill();

	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);
	virtual void Unsubscribed();
	virtual void Subscribed();
	void UpdateSkin();

	virtual unsigned short GetNetworkID() const = 0;

	virtual void TimeStepFixed(float dt);

private:
	void OnVehicleDestroy(GC_Object *sender, void *param);
	void OnVehicleKill(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerHuman : public GC_Player
{
protected:
	VehicleState _ctrlState;
public:
	GC_PlayerHuman();
	GC_PlayerHuman(FromFile);
	virtual ~GC_PlayerHuman() = 0;
	void SetControllerState(const VehicleState &vs);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerLocal : public GC_PlayerHuman
{
	DECLARE_SELF_REGISTRATION(GC_PlayerLocal);

	string_t _profile;
	std::deque<VehicleState> _stateHistory;


	//
	// cached values from the profile
	//

	int _keyForward;
	int _keyBack;
	int _keyLeft;
	int _keyRight;
	int _keyFire;
	int _keyLight;
	int _keyTowerLeft;
	int _keyTowerRight;
	int _keyTowerCenter;
	int _keyPickup;
	bool _aimToMouse;
	bool _moveToMouse;
	bool _arcadeStyle;


	//
	// controller state
	//

	mutable bool _lastLightKeyState;
	mutable bool _lastLightsState;

protected:
	class MyPropertySet : public GC_Player::MyPropertySet
	{
		typedef GC_Player::MyPropertySet BASE;

		ObjectProperty _propProfile;

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void MyExchange(bool applyToObject);
	};
	virtual PropertySet* NewPropertySet();


public:
	GC_PlayerLocal();
	GC_PlayerLocal(FromFile);
	virtual ~GC_PlayerLocal();

	void SelectFreeProfile();
	void ReadControllerStateAndStepPredicted(VehicleState &vs, float dt);

	virtual unsigned short GetNetworkID() const;

	virtual void TimeStepFixed(float dt);
	virtual void Serialize(SaveFile &f);
	virtual void MapExchange(MapFile &f);

	void SetProfile(const string_t &name);
	const string_t& GetProfile() const;
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerRemote : public GC_PlayerHuman
{
	DECLARE_SELF_REGISTRATION(GC_PlayerRemote);
	unsigned short _networkId;

public:
	GC_PlayerRemote(unsigned short id);
	GC_PlayerRemote(FromFile);
	virtual ~GC_PlayerRemote();

	virtual unsigned short GetNetworkID() const { return _networkId; }

	virtual void TimeStepFixed(float dt);
	virtual void Serialize(SaveFile &f);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

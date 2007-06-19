// Player.h

#pragma once

#include "Service.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

struct VehicleState;
class GC_Vehicle;

///////////////////////////////////////////////////////////////////////////////

class GC_Player : public GC_Service
{
	DECLARE_SELF_REGISTRATION(GC_Player);
	MemberOfGlobalList _memberOf;

	float     _timeRespawn;

	int       _team;
	int       _score;
	string_t  _nick;
	string_t  _class;
	string_t  _skin;

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

	public:
		MyPropertySet(GC_Object *object);
		virtual int GetCount() const;
		virtual ObjectProperty* GetProperty(int index);
		virtual void Exchange(bool applyToObject);
	};

protected:
	virtual void OnRespawn();
	virtual void OnDie();


public:
	bool IsDead() const { return _vehicle == NULL; }
	GC_Vehicle* GetVehicle() const { return GetRawPtr(_vehicle); }
	int GetTeam() const { return _team; }
	int GetScore() const { return _score; }
	const string_t& GetNick() const { return _nick; }
	const string_t& GetClass() const { return _class; }

	void SetSkin(const string_t &skin);
	void SetNick(const string_t &nick);
	void SetClass(const string_t &c);
	void SetTeam(int team);

	void ChangeScore(int delta);

public:
	GC_Player();
	GC_Player(FromFile);
	virtual ~GC_Player();
	virtual void Kill();

	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	void UpdateSkin();
	void ResetClass();

	virtual void TimeStepFixed(float dt);

private:
	void OnVehicleKill(GC_Object *sender, void *param);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerLocal : public GC_Player
{
	DECLARE_SELF_REGISTRATION(GC_PlayerLocal);

	string_t _profile;


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

	void GetControl(VehicleState &vs);

public:
	GC_PlayerLocal();
	GC_PlayerLocal(FromFile);
	virtual ~GC_PlayerLocal();

	virtual void TimeStepFixed(float dt);
	virtual void Serialize(SaveFile &f);

	void SetProfile(const char *name);
};

///////////////////////////////////////////////////////////////////////////////

class GC_PlayerRemote : public GC_Player
{
	DECLARE_SELF_REGISTRATION(GC_PlayerRemote);

	DWORD _networkId;
public:
	GC_PlayerRemote();
	GC_PlayerRemote(FromFile);
	virtual ~GC_PlayerRemote();

	DWORD GetNetworkId() const { return _networkId; }

	virtual void TimeStepFixed(float dt);
	virtual void Serialize(SaveFile &f);
};

///////////////////////////////////////////////////////////////////////////////
// end of file

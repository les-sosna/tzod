// Player.h

#pragma once

#include "Object.h"

///////////////////////////////////////////////////////////////////////////////
// forward declarations

class CController;
class GC_Vehicle;

///////////////////////////////////////////////////////////////////////////////

class GC_Player : public GC_Object
{
	DECLARE_SELF_REGISTRATION(GC_Player);
	MemberOfGlobalList _memberOf;

	float _time_respawn;

protected:
	CController* CreateController(int index);

public:
	CController		*_controller;
	int				_nIndex; // номер в g_options.players[]
	DWORD			_networkId;

//	char _name[MAX_PLRNAME];
//	char _skin[MAX_PATH];
//	char _class[MAX_VEHCLSNAME];

	string_t _name;
	string_t _skin;
	string_t _class;


public:
	SafePtr<GC_Vehicle>	_vehicle;
	bool dead() const
	{
		return _vehicle == NULL;
	}

public:
	int _team;
	int _score;

public:
	GC_Player(int nTeam);
	GC_Player(FromFile);
	virtual ~GC_Player();
	virtual void Kill();

	virtual bool IsSaved() { return true; };
	virtual void Serialize(SaveFile &f);

	void UpdateSkin();
	void Respawn();
	void SetController(int nIndex);

	void ResetClass();

	virtual void TimeStepFixed(float dt);

	void OnVehicleKill(GC_Object *sender, void *param);
};

// end of file

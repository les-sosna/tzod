#pragma once
#include "Service.h"
#include "ObjPtr.h"
#include <deque>

class GC_Vehicle;

#define MAX_TEAMS 6 // including 0 (no team)
#define PLAYER_RESPAWN_DELAY 2.0f

#define GC_FLAG_PLAYER_ISHUMAN     (GC_FLAG_SERVICE_ << 0)
#define GC_FLAG_PLAYER_ISACTIVE    (GC_FLAG_SERVICE_ << 1)
#define GC_FLAG_PLAYER_            (GC_FLAG_SERVICE_ << 2)

class GC_Player : public GC_Service
{
	DECLARE_SELF_REGISTRATION(GC_Player);
	DECLARE_LIST_MEMBER(override);

public:
	GC_Player();
	GC_Player(FromFile);
	virtual ~GC_Player();

	GC_Vehicle* GetVehicle() const { return _vehicle; }
	std::string_view GetOnDie() const { return _scriptOnDie; }
	std::string_view GetOnRespawn() const { return _scriptOnRespawn; }
	float GetDieTime() const { return _timeVehicleDestroyed; }

	void SetSkin(std::string skin);
	std::string_view GetSkin() const { return _skin; }

	void SetNick(std::string nick);
	std::string_view GetNick() const { return _nick; }

	void SetClass(std::string c);
	std::string_view GetClass() const { return _class; }

	void SetTeam(int team);
	int GetTeam() const { return _team; }

	void SetScore(int score);
	int GetScore() const { return _score; }

	void SetNumDeaths(int numDeaths) { _numDeaths = numDeaths; }
	int GetNumDeaths() const { return _numDeaths; }

	void SetIsHuman(bool isHuman) { SetFlags(GC_FLAG_PLAYER_ISHUMAN, isHuman); }
	bool GetIsHuman() const { return CheckFlags(GC_FLAG_PLAYER_ISHUMAN); }

	void SetIsActive(bool isActive) { SetFlags(GC_FLAG_PLAYER_ISACTIVE, isActive); }
	bool GetIsActive() const { return CheckFlags(GC_FLAG_PLAYER_ISACTIVE); }

	// GC_Object
	void Kill(World &world) override;
	void Serialize(World &world, SaveFile &f) override;
	void MapExchange(MapFile &f) override;
	void Init(World &world) override;
	void Resume(World &world) override;

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
		virtual void MyExchange(World &world, bool applyToObject);
	};
	PropertySet* NewPropertySet() override;

private:
	int _team = 0;
	int _score = 0;
	int _numDeaths = 0;
	std::string _nick;
	std::string _class;
	std::string _skin;
	std::string _vehname;
	std::string _scriptOnDie;
	std::string _scriptOnRespawn;
	ObjPtr<GC_Vehicle> _vehicle;
	float _timeVehicleDestroyed = 0;

	friend class GC_Vehicle;
	void OnVehicleDestroy(World &world);
};

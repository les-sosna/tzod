#pragma once
#include "Service.h"
#include <deque>

class GC_Vehicle;

#define GC_FLAG_PLAYER_ISHUMAN     (GC_FLAG_SERVICE_ << 0)
#define GC_FLAG_PLAYER_            (GC_FLAG_SERVICE_ << 1)

class GC_Player : public GC_Service
{
    DECLARE_SELF_REGISTRATION(GC_Player);
    typedef GC_Service base;
    
	float     _timeRespawn;

	int       _team;
	int       _score;
	std::string  _nick;
	std::string  _class;
	std::string  _skin;
	std::string  _vehname;
	std::string  _scriptOnDie;
	std::string  _scriptOnRespawn;

	ObjPtr<GC_Vehicle> _vehicle;

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
	virtual PropertySet* NewPropertySet();

protected:
	virtual void OnRespawn();
	virtual void OnDie();

public:
    DECLARE_LIST_MEMBER();
    
	GC_Vehicle* GetVehicle() const { return _vehicle; }

	void SetSkin(const std::string &skin);
	const std::string& GetSkin() const { return _skin; }

	void SetNick(const std::string &nick);
	const std::string& GetNick() const { return _nick; }

	void SetClass(const std::string &c);
	const std::string& GetClass() const { return _class; }

	void SetTeam(int team);
	int GetTeam() const { return _team; }

	void SetScore(World &world, int score);
	int GetScore() const { return _score; }

    void SetIsHuman(bool isHuman) { SetFlags(GC_FLAG_PLAYER_ISHUMAN, isHuman); }
    bool GetIsHuman() const { return CheckFlags(GC_FLAG_PLAYER_ISHUMAN); }

public:
	GC_Player(World &world);
	GC_Player(FromFile);
	virtual ~GC_Player();
	void UpdateSkin();

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(World &world, MapFile &f);
	virtual void TimeStepFixed(World &world, float dt);

private:
	void OnVehicleDestroy(World &world, GC_Object *sender, void *param);
	void OnVehicleKill(World &world, GC_Object *sender, void *param);
};

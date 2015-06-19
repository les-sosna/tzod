#pragma once
#include "Service.h"
#include "ObjPtr.h"
#include <deque>

class GC_Camera;
class GC_Vehicle;

#define MAX_TEAMS 6 // including 0 (no team)
#define PLAYER_RESPAWN_DELAY 2.0f

#define GC_FLAG_PLAYER_ISHUMAN     (GC_FLAG_SERVICE_ << 0)
#define GC_FLAG_PLAYER_            (GC_FLAG_SERVICE_ << 1)

class GC_Player : public GC_Service
{
    DECLARE_SELF_REGISTRATION(GC_Player);
	DECLARE_LIST_MEMBER();
    typedef GC_Service base;
	
public:
	GC_Player();
	GC_Player(FromFile);
	virtual ~GC_Player();

	GC_Vehicle* GetVehicle() const { return _vehicle; }
	const std::string& GetOnDie() const { return _scriptOnDie; }
	const std::string& GetOnRespawn() const { return _scriptOnRespawn; }
	
	void SetSkin(const std::string &skin);
	const std::string& GetSkin() const { return _skin; }
	
	void SetNick(const std::string &nick);
	const std::string& GetNick() const { return _nick; }
	
	void SetClass(const std::string &c);
	const std::string& GetClass() const { return _class; }
	
	void SetTeam(int team);
	int GetTeam() const { return _team; }
	
	void SetScore(int score);
	int GetScore() const { return _score; }
	
	void SetIsHuman(bool isHuman) { SetFlags(GC_FLAG_PLAYER_ISHUMAN, isHuman); }
	bool GetIsHuman() const { return CheckFlags(GC_FLAG_PLAYER_ISHUMAN); }
	
	void SetCamera(GC_Camera *camera) { _camera = camera; }

	// GC_Object
	virtual void Kill(World &world);
	virtual void Serialize(World &world, SaveFile &f);
	virtual void MapExchange(MapFile &f);
	virtual void TimeStep(World &world, float dt);

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

private:
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
	ObjPtr<GC_Camera> _camera;
	
	friend class GC_Vehicle;
	void OnVehicleDestroy(World &world);
};

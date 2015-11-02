#pragma once
#include <unordered_set>

class GC_Player;

enum class MurderType
{
	Accident,
	Enemy,
	Friend,
	Suicide,
};

struct GameListener
{
	virtual void OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType) = 0;
};

struct GameEventSource
{
	virtual void AddListener(GameListener &ls) = 0;
	virtual void RemoveListener(GameListener &ls) = 0;
};

class GameEventsBroadcaster
	: public GameEventSource
	, public GameListener
{
public:
	~GameEventsBroadcaster();

	// GameEventSource
	void AddListener(GameListener &ls) override;
	void RemoveListener(GameListener &ls) override;

	// GameListener
	void OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType) override;

private:
	std::unordered_set<GameListener*> _listeners;
};

#pragma once
#include <unordered_set>

struct GameListener
{
	virtual void OnGameMessage(const char *msg) = 0;
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
	void OnGameMessage(const char *msg) override;

private:
	std::unordered_set<GameListener*> _listeners;
};

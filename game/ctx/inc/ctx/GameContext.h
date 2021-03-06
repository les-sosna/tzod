#pragma once
#include "ScriptMessageBroadcaster.h"
#include "GameContextBase.h"
#include "GameEvents.h"
#include <functional>
#include <memory>
#include <string>
#include <vector>

class World;
struct Gameplay;

namespace FS
{
	struct Stream;
}

class AIManager;
class ScriptHarness;
class ThemeManager;
class TextureManager;
class WorldController;

struct PlayerDesc
{
	std::string nick;
	std::string skin;
	std::string cls;
	unsigned int team;
};

enum class AIDiffuculty;

struct DMSettings
{
	std::vector<PlayerDesc> players;
	std::vector<PlayerDesc> bots;
	AIDiffuculty difficulty{};
	int fragLimit{};
	float timeLimit{};
};


class GameContext : public GameContextBase
{
public:
	GameContext(std::unique_ptr<World> world, const DMSettings &settings);
	virtual ~GameContext();
	WorldController& GetWorldController() { return *_worldController; }
	const WorldController& GetWorldController() const { return *_worldController; }
	GameEventSource& GetGameEventSource() { return _gameEventsBroadcaster; }
	ScriptMessageSource& GetScriptMessageSource() { return _scriptMessageBroadcaster; }
	AIManager& GetAIManager() { return *_aiManager; }
	AIDiffuculty GetDifficulty() const { return _difficulty; }
	float GetGameplayTime() const { return _gameplayTime; }
	bool IsGameplayActive() const;

	void Serialize(FS::Stream &stream);
	void Deserialize(FS::Stream &stream);

	// GameContextBase
	World& GetWorld() override { return *_world; }
	Gameplay* GetGameplay() const override;
	void Step(float dt, AppConfig &appConfig, bool *outConfigChanged) override;
	bool IsWorldActive() const override;

private:
	GameEventsBroadcaster _gameEventsBroadcaster;
	app_detail::ScriptMessageBroadcaster _scriptMessageBroadcaster;
	std::unique_ptr<World> _world;
	std::unique_ptr<WorldController> _worldController;
	std::unique_ptr<Gameplay> _gameplay;
	std::unique_ptr<ScriptHarness> _scriptHarness;
	std::unique_ptr<AIManager> _aiManager;
	const AIDiffuculty _difficulty;
	float _gameplayTime = 0;
};

class GameContextCampaignDM final
	: public GameContext
{
public:
	GameContextCampaignDM(std::unique_ptr<World> world, const DMSettings &settings, int campaignTier, int campaignMap);

	int GetRating() const;

	// GameContextBase
	void Step(float dt, AppConfig &appConfig, bool *outConfigChanged) override;

private:
	bool IsVictory() const;
	const int _campaignTier;
	const int _campaignMap;
};

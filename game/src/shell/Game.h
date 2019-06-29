#pragma once
#include "CampaignControlCommands.h"
#include "InputManager.h"
#include <ctx/GameEvents.h>
#include <gv/GameViewHarness.h>
#include <ui/PointerInput.h>
#include <ui/Text.h>
#include <ui/Texture.h>
#include <ui/Window.h>
#include <unordered_map>

class ShellConfig;
class LangCache;
class GameContext;
class World;
class WorldView;
class WorldController;

class MessageArea;
class ScoreTable;
class CampaignControls;


namespace Plat
{
	class ConsoleBuffer;
}

namespace UI
{
	class Rating;
	class Rectangle;
	class StackLayout;
}

class GameLayout final
	: public UI::WindowContainer
	, public UI::TimeStepping
	, private UI::PointerSink
	, private GameListener
{
public:
	GameLayout(UI::TimeStepManager &manager,
	           std::shared_ptr<GameContext> gameContext,
	           WorldView &worldView,
	           WorldController &worldController,
	           ShellConfig &conf,
	           LangCache &lang,
	           Plat::ConsoleBuffer &logger,
	           CampaignControlCommands campaignControlCommands);
	virtual ~GameLayout();

	// Window
	void OnTimeStep(Plat::Input &input, bool focused, float dt) override;
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, float time, bool hovered) const override;
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	PointerSink* GetPointerSink() override { return this; }
	std::shared_ptr<const UI::Window> GetFocus(const std::shared_ptr<const UI::Window>& owner) const override;
	const UI::Window* GetFocus() const override;

private:
	vec2d GetDragDirection() const;
	unsigned int GetEffectiveDragCount() const;
	float GetLastPlayerDieTime() const;
	bool GetAllPlayerDead() const;

	std::shared_ptr<MessageArea> _msg;
	std::shared_ptr<UI::Rectangle> _background;
	std::shared_ptr<UI::StackLayout> _scoreAndControls;
	std::shared_ptr<UI::Rating> _rating;
	std::shared_ptr<ScoreTable> _score;
	std::shared_ptr<CampaignControls> _campaignControls;
	std::shared_ptr<UI::Text> _timerDisplay;

	std::shared_ptr<GameContext> _gameContext;
	GameViewHarness _gameViewHarness;
	WorldView &_worldView;
	WorldController &_worldController;
	ShellConfig &_conf;
	LangCache &_lang;
	InputManager _inputMgr;
	UI::Texture _texDrag = "ui/direction";
	UI::Texture _texTarget = "ui/target";

	std::unordered_map<unsigned int, std::pair<vec2d, vec2d>> _activeDrags;

	// UI::PointerSink
	bool OnPointerDown(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerUp(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerMove(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured) override;
	void OnTap(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// GameListener
	void OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType) override;
};

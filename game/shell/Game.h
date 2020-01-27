#pragma once
#include "CampaignControlCommands.h"
#include "InputManager.h"
#include <ctx/GameEvents.h>
#include <gv/GameViewHarness.h>
#include <ui/Navigation.h>
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
class GamePauseMenu;


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
	, private UI::KeyboardSink
	, private UI::NavigationSink
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

	vec2d GetListenerPos() const;

	void ShowPauseMenu();
	bool CanPause() const;

	void NavigateBack();
	bool CanNavigateBack() const;

	bool GetGameOver() const;

	// Window
	void OnTimeStep(const Plat::Input &input, bool focused, float dt) override;
	void Draw(const UI::DataContext &dc, const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, RenderContext &rc, TextureManager &texman, const Plat::Input &input, float time, bool hovered) const override;
	UI::WindowLayout GetChildLayout(TextureManager &texman, const UI::LayoutContext &lc, const UI::DataContext &dc, const UI::Window &child) const override;
	vec2d GetContentSize(TextureManager& texman, const UI::DataContext& dc, float scale, const UI::LayoutConstraints& layoutConstraints) const override;
	PointerSink* GetPointerSink() override { return this; }
	std::shared_ptr<const UI::Window> GetFocus(const std::shared_ptr<const UI::Window>& owner) const override;
	const UI::Window* GetFocus() const override;
	UI::NavigationSink* GetNavigationSink() override { return this; }
	UI::KeyboardSink* GetKeyboardSink() override { return this; }

private:
	vec2d GetDragDirection() const;
	unsigned int GetEffectiveDragCount() const;
	float GetLastPlayerDieTime() const;
	bool GetAllPlayerDead() const;

	std::shared_ptr<MessageArea> _msg;
	std::shared_ptr<UI::Text> _timerDisplay;
	std::shared_ptr<UI::Rectangle> _background;
	std::shared_ptr<UI::StackLayout> _scoreAndControls;
	std::shared_ptr<UI::Rating> _rating;
	std::shared_ptr<ScoreTable> _score;
	std::shared_ptr<CampaignControls> _campaignControls;
	std::shared_ptr<GamePauseMenu> _gamePauseMenu;

	std::shared_ptr<GameContext> _gameContext;
	GameViewHarness _gameViewHarness;
	WorldView &_worldView;
	WorldController &_worldController;
	ShellConfig &_conf;
	LangCache &_lang;
	CampaignControlCommands _campaignControlCommands;
	InputManager _inputMgr;
	UI::Texture _texDrag = "ui/direction";
	UI::Texture _texTarget = "ui/target";

	std::unordered_map<unsigned int, std::pair<vec2d, vec2d>> _activeDrags;

	// UI::PointerSink
	bool OnPointerDown(const Plat::Input &input, const  UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerUp(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, int button) override;
	void OnPointerMove(const Plat::Input &input, const  UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, UI::PointerInfo pi, bool captured) override;
	void OnTap(const UI::InputContext &ic, const UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// UI::KeyboardSink
	bool OnKeyPressed(const Plat::Input& input, const UI::InputContext& ic, Plat::Key key) override;

	// UI::NavigationSink
	bool CanNavigate(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate) const override;
	void OnNavigate(TextureManager& texman, const UI::LayoutContext& lc, const UI::DataContext& dc, UI::Navigate navigate, UI::NavigationPhase phase) override;

	// GameListener
	void OnMurder(GC_Player& victim, GC_Player* killer, MurderType murderType) override;
};

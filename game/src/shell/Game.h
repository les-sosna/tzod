#pragma once
#include "InputManager.h"
#include <ctx/GameEvents.h>
#include <gv/GameViewHarness.h>
#include <ui/Window.h>
#include <ui/Text.h>
#include <unordered_map>

class ConfCache;
class LangCache;
class GameContext;
class World;
class WorldView;
class WorldController;
class DefaultCamera;

class MessageArea;
class ScoreTable;
class CampaignControls;

namespace UI
{
	class ConsoleBuffer;
	class StackLayout;
}

class GameLayout
	: public UI::Window
	, private UI::PointerSink
	, private GameListener
{
public:
	GameLayout(UI::LayoutManager &manager,
	           TextureManager &texman,
	           GameContext &gameContext,
	           WorldView &worldView,
	           WorldController &worldController,
	           ConfCache &conf,
	           LangCache &lang,
	           UI::ConsoleBuffer &logger);
	virtual ~GameLayout();

	// Window
	void OnTimeStep(UI::LayoutManager &manager, float dt) override;
	void Draw(const UI::StateContext &sc, const UI::LayoutContext &lc, const UI::InputContext &ic, DrawingContext &dc, TextureManager &texman) const override;
	FRECT GetChildRect(TextureManager &texman, const UI::LayoutContext &lc, const UI::StateContext &sc, const UI::Window &child) const override;
	PointerSink* GetPointerSink() override { return this; }

private:
	void OnChangeShowTime();
	vec2d GetDragDirection() const;
	unsigned int GetEffectiveDragCount() const;

	std::shared_ptr<MessageArea> _msg;
	std::shared_ptr<UI::StackLayout> _scoreAndControls;
	std::shared_ptr<ScoreTable> _score;
	std::shared_ptr<CampaignControls> _campaignControls;
	std::shared_ptr<UI::Text> _timerDisplay;

	GameContext &_gameContext;
	GameViewHarness _gameViewHarness;
	WorldView &_worldView;
	WorldController &_worldController;
	ConfCache &_conf;
	LangCache &_lang;
	InputManager _inputMgr;
	size_t _texDrag;
	size_t _texTarget;

	std::unordered_map<unsigned int, std::pair<vec2d, vec2d>> _activeDrags;

	// UI::PointerSink
	bool OnPointerDown(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerUp(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, int button, UI::PointerType pointerType, unsigned int pointerID) override;
	void OnPointerMove(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition, UI::PointerType pointerType, unsigned int pointerID, bool captured) override;
	void OnTap(UI::InputContext &ic, UI::LayoutContext &lc, TextureManager &texman, vec2d pointerPosition) override;

	// GameListener
	void OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType) override;
};

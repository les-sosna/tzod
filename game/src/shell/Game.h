#pragma once
#include "InputManager.h"
#include <ctx/GameEvents.h>
#include <gv/GameViewHarness.h>
#include <ui/Window.h>
#include <ui/Text.h>

class ConfCache;
class LangCache;
class GameContext;
class World;
class WorldView;
class WorldController;
class DefaultCamera;

class MessageArea;
class ScoreTable;

class TimeElapsed : public UI::Text
{
public:
	TimeElapsed(UI::Window *parent, float x, float y, enumAlignText align, World &world);

private:
	void OnVisibleChange(bool visible, bool inherited) override;
	void OnTimeStep(float dt) override;

	World &_world;
};

namespace UI
{
	class ConsoleBuffer;
}

class GameLayout
	: public UI::Window
	, private GameListener
{
public:
	GameLayout(UI::Window *parent,
	           GameContext &gameContext,
	           WorldView &worldView,
	           WorldController &worldController,
	           const DefaultCamera &defaultCamera,
	           ConfCache &conf,
	           LangCache &lang,
	           UI::ConsoleBuffer &logger);
	virtual ~GameLayout();

	// Window
	void OnTimeStep(float dt) override;
	void DrawChildren(DrawingContext &dc, float sx, float sy) const override;
	void OnSize(float width, float height) override;
	bool OnFocus(bool focus) override { return true; }
    bool OnPointerDown (float x, float y, int button, UI::PointerType pointerType, unsigned int pointerID) override;
    bool OnPointerUp   (float x, float y, int button, UI::PointerType pointerType, unsigned int pointerID) override;
    bool OnPointerMove (float x, float y, UI::PointerType pointerType, unsigned int pointerID) override;

private:
	void OnChangeShowTime();

	MessageArea  *_msg;
	ScoreTable   *_score;
	TimeElapsed  *_time;

	GameContext &_gameContext;
	GameViewHarness _gameViewHarness;
	WorldView &_worldView;
	WorldController &_worldController;
	const DefaultCamera &_defaultCamera;
	ConfCache &_conf;
	LangCache &_lang;
	InputManager _inputMgr;
    
    vec2d _dragOrigin;
    vec2d _dragDirection;
    int _fire;

	// GameListener
	void OnMurder(GC_Player &victim, GC_Player *killer, MurderType murderType) override;
};

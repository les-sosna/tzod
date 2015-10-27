#pragma once
#include "detail/InputManager.h"
#include <app/GameEvents.h>
#include <app/GameViewHarness.h>
#include <ui/Window.h>
#include <ui/Text.h>

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
	           UI::ConsoleBuffer &logger);
	virtual ~GameLayout();

	// Window
	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;
	virtual void OnSize(float width, float height);
	virtual bool OnFocus(bool focus) { return true; }

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
	InputManager _inputMgr;

	// GameListener
	void OnGameMessage(const char *msg) override;
};

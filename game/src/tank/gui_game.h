#pragma once
#include "InputManager.h"
#include <app/GameEvents.h>
#include <app/GameViewHarness.h>
#include <ui/Window.h>
#include <ui/Text.h>

class GameContext;
class World;
class WorldView;
class WorldController;
class DefaultCamera;

namespace UI
{
    class MessageArea;
    class ScoreTable;

class TimeElapsed : public Text
{
public:
	TimeElapsed(Window *parent, float x, float y, enumAlignText align, World &world);

private:
	void OnVisibleChange(bool visible, bool inherited) override;
	void OnTimeStep(float dt) override;

    World &_world;
};

class GameLayout
    : public Window
	, private GameListener
{
public:
    GameLayout(Window *parent,
               GameContext &gameContext,
			   WorldView &worldView,
			   WorldController &worldController,
			   const DefaultCamera &defaultCamera);
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
    InputManager _inputMgr;

	// GameListener
	void OnGameMessage(const char *msg) override;
};


} // end of namespace UI

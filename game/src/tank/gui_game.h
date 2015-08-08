#pragma once
#include "InputManager.h"
#include <app/GameEvents.h>
#include <app/GameViewHarness.h>
#include <ui/Window.h>
#include <ui/Text.h>

class World;
class WorldView;
class WorldController;
class DefaultCamera;
struct Gameplay;

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
			   GameEventSource &gameEventSource,
			   World &world,
			   WorldView &worldView,
			   WorldController &worldController,
			   Gameplay &gameplay,
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

	GameEventSource &_gameEventSource;
    GameViewHarness _gameViewHarness;
    World &_world;
    WorldView &_worldView;
	WorldController &_worldController;
	Gameplay &_gameplay;
    const DefaultCamera &_defaultCamera;
    InputManager _inputMgr;

	// GameListener
	void OnGameMessage(const char *msg) override;
};


} // end of namespace UI

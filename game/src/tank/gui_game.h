#pragma once

#include "gc/WorldEvents.h"

#include <Window.h>
#include <Text.h>

class World;
class WorldView;
class InputManager;
class DefaultCamera;

namespace UI
{
    class MessageArea;
    class ScoreTable;
    class TimeElapsed;

class TimeElapsed : public Text
{
    World &_world;
public:
	TimeElapsed(Window *parent, float x, float y, enumAlignText align, World &world);
protected:
	void OnVisibleChange(bool visible, bool inherited);
	void OnTimeStep(float dt);
};

class GameLayout
    : public Window
    , private MessageListener
{
public:
    GameLayout(Window *parent, World &world, WorldView &worldView, InputManager &inputMgr, const DefaultCamera &defaultCamera);
    virtual ~GameLayout();
    
    // Window
	virtual void OnTimeStep(float dt);
	virtual void DrawChildren(DrawingContext &dc, float sx, float sy) const;
	virtual void OnSize(float width, float height);
	virtual bool OnFocus(bool focus) { return true; }

private:
	void OnChangeShowTime();
    virtual void OnGameMessage(const char *msg);  // MessageListener

	MessageArea  *_msg;
	ScoreTable   *_score;
	TimeElapsed  *_time;

    World &_world;
    WorldView &_worldView;
	InputManager &_inputMgr;
    const DefaultCamera &_defaultCamera;
};


} // end of namespace UI

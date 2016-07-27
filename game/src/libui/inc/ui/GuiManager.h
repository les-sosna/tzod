#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <list>
#include <memory>
#include <vector>

class DrawingContext;
class TextureManager;

namespace UI
{

class InputContext;
class LayoutContext;
class StateContext;
class Window;

struct RenderSettings
{
	StateContext &sc;
	LayoutContext &lc;
	InputContext &ic;
	DrawingContext &dc;
	TextureManager &texman;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> hoverPath;
};

void RenderUIRoot(Window &desktop, RenderSettings &rs);

class LayoutManager
{
public:
	LayoutManager(TextureManager &texman, InputContext &ic);
	~LayoutManager();

	void TimeStep(float dt);

	InputContext& GetInputContext() const { return _inputContext; }
	TextureManager& GetTextureManager() const { return _texman; }
	float GetTime() const { return _time; }

	std::shared_ptr<Window> GetDesktop() const { return _desktop; }
	void SetDesktop(std::shared_ptr<Window> desktop) { _desktop = std::move(desktop); }

private:
	friend class Window;
	std::list<Window*>::iterator TimeStepRegister(Window* wnd);
	void TimeStepUnregister(std::list<Window*>::iterator it);

	TextureManager &_texman;
	InputContext &_inputContext;

	std::list<Window*> _timestep;
	std::list<Window*>::iterator _tsCurrent;
	bool _tsDeleteCurrent;
	float _time = 0;

	std::shared_ptr<Window> _desktop;
};

} // namespace UI

#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <list>
#include <memory>
#include <vector>

class RenderContext;
class TextureManager;

namespace UI
{

class DataContext;
class InputContext;
class LayoutContext;
class Managerful;
class StateContext;
class Window;

struct RenderSettings
{
	InputContext &ic;
	RenderContext &rc;
	TextureManager &texman;
	bool topMostPass;
	std::vector<std::shared_ptr<Window>> hoverPath;
};

void RenderUIRoot(Window &desktop, RenderSettings &rs, const LayoutContext &lc, const DataContext &dc, const StateContext &sc);

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
	friend class Managerful;
	std::list<Managerful*>::iterator TimeStepRegister(Managerful* wnd);
	void TimeStepUnregister(std::list<Managerful*>::iterator it);

	TextureManager &_texman;
	InputContext &_inputContext;

	std::list<Managerful*> _timestep;
	std::list<Managerful*>::iterator _tsCurrent;
	bool _tsDeleteCurrent;
	float _time = 0;

	std::shared_ptr<Window> _desktop;
};

} // namespace UI

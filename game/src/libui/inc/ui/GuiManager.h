#pragma once
#include "Pointers.h"
#include <math/MyMath.h>
#include <list>
#include <memory>

class DrawingContext;
class TextureManager;

namespace UI
{

class InputContext;
class Window;

class LayoutManager
{
public:
	LayoutManager(TextureManager &texman, InputContext &ic);
	~LayoutManager();

	void TimeStep(float dt);
	void Render(vec2d size, DrawingContext &dc) const;

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

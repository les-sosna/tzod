#include "inc/ui/Window.h"
#include "inc/ui/WindowPtr.h"

using namespace UI;
using namespace UI::detail;

WindowWeakPtr::WindowWeakPtr(Window *p)
	: _resident(p ? p->_resident : nullptr)
{
	assert(!p || _resident);
	assert(!_resident || _resident->ptr);
	if (_resident) _resident->counter++;
}

WindowWeakPtr::~WindowWeakPtr()
{
	Set(nullptr);
}

Window* WindowWeakPtr::operator->() const
{
	assert(_resident && _resident->ptr);
	return _resident->ptr;
}

Window* WindowWeakPtr::Get() const
{
	return _resident ? _resident->ptr : nullptr;
}

void WindowWeakPtr::Set(Window *p)
{
	assert(!_resident || _resident->counter > 0);
	if (_resident && 0 == --_resident->counter && !_resident->ptr)
	{
		delete _resident;
	}

	_resident = p ? p->_resident : nullptr;
	assert(!p || _resident);
	assert(!_resident || _resident->ptr);
	if (_resident) _resident->counter++;
}

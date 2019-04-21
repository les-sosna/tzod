#include "inc/platglfw/GlfwAppWindow.h"
#include "inc/platglfw/GlfwPlatform.h"
#include "inc/platglfw/GlfwKeys.h"
#include <plat/Input.h>
#include <video/RenderOpenGL.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

static Plat::AppWindowInputSink* s_inputSink;

unsigned int GlfwInitHelper::s_initCount = 0;

GlfwInitHelper::GlfwInitHelper()
	: _isActive(true)
{
	if (s_initCount == 0)
	{
		if (!glfwInit())
			throw std::runtime_error("Failed to initialize OpenGL");
	}
	++s_initCount;
}

GlfwInitHelper::~GlfwInitHelper()
{
	if (_isActive)
	{
		assert(s_initCount > 0);
		--s_initCount;
		if (s_initCount == 0)
		{
			glfwTerminate();
		}
	}
}

GlfwInitHelper::GlfwInitHelper(GlfwInitHelper &&other) noexcept
	: _isActive(true)
{
	assert(other._isActive);
	other._isActive = false;
}

void GlfwCursorDeleter::operator()(GLFWcursor *cursor)
{
	glfwDestroyCursor(cursor);
}

void GlfwWindowDeleter::operator()(GLFWwindow *window)
{
	glfwDestroyWindow(window);
}

static float GetLayoutScale(GLFWwindow *window)
{
	int framebuferWidth;
	glfwGetFramebufferSize(window, &framebuferWidth, nullptr);

	int logicalWidth;
	glfwGetWindowSize(window, &logicalWidth, nullptr);

	return logicalWidth > 0 ? (float)framebuferWidth / (float)logicalWidth : 1.f;
}

static void OnMouseButton(GLFWwindow *window, int platformButton, int platformAction, int mods)
{
	assert(s_inputSink);
	if( auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window) )
	{
		Plat::Msg action = (GLFW_RELEASE == platformAction) ? Plat::Msg::PointerUp : Plat::Msg::PointerDown;
		int buttons = 0;
		switch (platformButton)
		{
			case GLFW_MOUSE_BUTTON_LEFT:
				buttons |= 0x01;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				buttons |= 0x02;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				buttons |= 0x04;
				break;
			default:
				return;
		}
		vec2d pxMousePos = GetCursorPosInPixels(window);
		s_inputSink->OnPointer(*self, Plat::PointerType::Mouse, action, pxMousePos, vec2d{} /*offset*/, buttons, 0 /*pointerID*/);
	}
}

static void OnCursorEnter(GLFWwindow* window, int entered)
{
	assert(s_inputSink);
	if (auto self = (GlfwAppWindow*)glfwGetWindowUserPointer(window))
	{
		self->GetInput().SetMousePresent(!!entered);
	}
}

static void OnCursorPos(GLFWwindow *window, double xpos, double ypos)
{
	assert(s_inputSink);
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		vec2d pxMousePos = GetCursorPosInPixels(window, xpos, ypos);
		s_inputSink->OnPointer(*self, Plat::PointerType::Mouse, Plat::Msg::PointerMove, pxMousePos, vec2d{}/*offset*/, 0 /*buttons*/, 0 /*pointerID*/);
	}
}

static void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
	assert(s_inputSink);
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		vec2d pxMousePos = GetCursorPosInPixels(window);
		vec2d pxMouseOffset = GetCursorPosInPixels(window, xoffset, yoffset);
		auto msg = Plat::Msg::Scroll;
#ifdef __APPLE__
		pxMouseOffset *= 10;
		msg = Plat::Msg::ScrollPrecise;
#endif
		s_inputSink->OnPointer(*self, Plat::PointerType::Mouse, msg, pxMousePos, pxMouseOffset, 0 /*buttons*/, 0 /*pointerID*/);
	}
}

static GLFWmonitor* GetCurrentMonitor(GLFWwindow* window)
{
	GLFWmonitor* bestMonitor = glfwGetPrimaryMonitor();
	int bestOverlap = 0;

	int wx, wy, ww, wh;
	glfwGetWindowPos(window, &wx, &wy);
	glfwGetWindowSize(window, &ww, &wh);

	int count = 0;
	GLFWmonitor** monitors = glfwGetMonitors(&count);
	for (int i = 0; i < count; i++)
	{
		int mx, my;
		glfwGetMonitorPos(monitors[i], &mx, &my);
		const GLFWvidmode* mode = glfwGetVideoMode(monitors[i]);

		int overlap = std::max(0, std::min(wx + ww, mx + mode->width) - std::max(wx, mx)) *
		              std::max(0, std::min(wy + wh, my + mode->height) - std::max(wy, my));

		if (bestOverlap < overlap)
		{
			bestOverlap = overlap;
			bestMonitor = monitors[i];
		}
	}

	return bestMonitor;
}

static void OnKey(GLFWwindow *window, int platformKey, int scancode, int platformAction, int mods)
{
	assert(s_inputSink);
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if (!!(mods & GLFW_MOD_ALT) && GLFW_KEY_ENTER == platformKey && GLFW_PRESS == platformAction)
		{
			if (GLFWmonitor * monitor = glfwGetWindowMonitor(window) ? nullptr : GetCurrentMonitor(window))
			{
				glfwGetWindowSize(window, &self->_windowedWidth, &self->_windowedHeight);
				glfwGetWindowPos(window, &self->_windowedLeft, &self->_windowedTop);
				glfwSetWindowMonitor(window, monitor, 0, 0,
					glfwGetVideoMode(monitor)->width, glfwGetVideoMode(monitor)->height, GLFW_DONT_CARE);
				glfwSwapInterval(1);
			}
			else
			{
				glfwSetWindowMonitor(window, nullptr,
					self->_windowedTop, self->_windowedLeft,
					self->_windowedWidth, self->_windowedHeight, GLFW_DONT_CARE);
			}
		}
		else
		{
			Plat::Key key = MapGlfwKeyCode(platformKey);
			Plat::Msg action = (GLFW_RELEASE == platformAction) ? Plat::Msg::KeyReleased : Plat::Msg::KeyPressed;
			s_inputSink->OnKey(*self, key, action);
		}
	}
}

static void OnChar(GLFWwindow *window, unsigned int codepoint)
{
	assert(s_inputSink);
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if( codepoint < 57344 || codepoint > 63743 ) // ignore Private Use Area characters
		{
			s_inputSink->OnChar(*self, codepoint);
		}
	}
}

static void OnRefresh(GLFWwindow *window)
{
	// Workaround: XAudio may spin message pump outside of the main loop during initialization
	if (!s_inputSink)
		return;

	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		s_inputSink->OnRefresh(*self);
	}
}

static std::unique_ptr<GLFWwindow, GlfwWindowDeleter> NewWindow(const char *title, bool fullscreen, int width, int height)
{
	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> result(glfwCreateWindow(
		fullscreen ? glfwGetVideoMode(glfwGetPrimaryMonitor())->width : width,
		fullscreen ? glfwGetVideoMode(glfwGetPrimaryMonitor())->height : height,
		title,
		fullscreen ? glfwGetPrimaryMonitor() : nullptr,
		nullptr));

	if (!result)
		throw std::runtime_error("Failed to create GLFW window");

	return result;
}

GlfwAppWindow::GlfwAppWindow(const char *title, bool fullscreen, int width, int height)
	: _window(NewWindow(title, fullscreen, width, height))
	, _cursorArrow(glfwCreateStandardCursor(GLFW_ARROW_CURSOR))
	, _cursorIBeam(glfwCreateStandardCursor(GLFW_IBEAM_CURSOR))
	, _clipboard(new GlfwClipboard(*_window))
	, _input(new GlfwInput(*_window))
	, _render(RenderCreateOpenGL())
{
	glfwSetMouseButtonCallback(_window.get(), OnMouseButton);
	glfwSetCursorEnterCallback(_window.get(), OnCursorEnter);
	glfwSetCursorPosCallback(_window.get(), OnCursorPos);
	glfwSetScrollCallback(_window.get(), OnScroll);
	glfwSetKeyCallback(_window.get(), OnKey);
	glfwSetCharCallback(_window.get(), OnChar);
	glfwSetWindowRefreshCallback(_window.get(), OnRefresh);

	glfwMakeContextCurrent(_window.get());
	glfwSwapInterval(1);

	glfwSetWindowUserPointer(_window.get(), this);
}

GlfwAppWindow::~GlfwAppWindow()
{
	glfwSetWindowUserPointer(_window.get(), nullptr);

	glfwMakeContextCurrent(_window.get());
	_render.reset();
	glfwMakeContextCurrent(nullptr);
}

Plat::Clipboard& GlfwAppWindow::GetClipboard()
{
	return *_clipboard;
}

GlfwInput& GlfwAppWindow::GetInput()
{
	return *_input;
}

IRender& GlfwAppWindow::GetRender()
{
	glfwMakeContextCurrent(_window.get());
	return *_render;
}

vec2d GlfwAppWindow::GetPixelSize() const
{
	int width;
	int height;
	glfwGetFramebufferSize(_window.get(), &width, &height);
	return vec2d{ (float)width, (float)height };
}

float GlfwAppWindow::GetLayoutScale() const
{
	return ::GetLayoutScale(_window.get());
}

void GlfwAppWindow::SetCanNavigateBack(bool canNavigateBack)
{
}

void GlfwAppWindow::SetMouseCursor(Plat::MouseCursor mouseCursor)
{
	switch (mouseCursor)
	{
	case Plat::MouseCursor::Arrow:
		glfwSetCursor(_window.get(), _cursorArrow.get());
		break;
	case Plat::MouseCursor::IBeam:
		glfwSetCursor(_window.get(), _cursorIBeam.get());
		break;
	default:
		glfwSetCursor(_window.get(), nullptr);
		break;
	}
}

void GlfwAppWindow::Present()
{
	assert(glfwGetCurrentContext() == _window.get());
	glfwSwapBuffers(_window.get());
	glFinish(); // prevent gpu queue growth to reduce input lag, only seems to help in full screen
}

bool GlfwAppWindow::ShouldClose() const
{
	return !!glfwWindowShouldClose(_window.get());
}

void GlfwAppWindow::PollEvents(Plat::AppWindowInputSink& inputSink) // static
{
	assert(!s_inputSink);
	s_inputSink = &inputSink;
	glfwPollEvents();
	s_inputSink = nullptr;
}

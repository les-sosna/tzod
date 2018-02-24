#include "inc/platglfw/GlfwAppWindow.h"
#include "inc/platglfw/GlfwPlatform.h"
#include "inc/platglfw/GlfwKeys.h"
#include <ui/InputContext.h>
#include <ui/PointerInput.h>
#include <video/RenderOpenGL.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

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

GlfwInitHelper::GlfwInitHelper(GlfwInitHelper &&other)
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
	if( auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window) )
	{
		if (auto inputSink = self->GetInputSink())
		{
			UI::Msg action = (GLFW_RELEASE == platformAction) ? UI::Msg::PointerUp : UI::Msg::PointerDown;
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
			inputSink->OnPointer(UI::PointerType::Mouse, action, pxMousePos, vec2d{} /*offset*/, buttons, 0 /*pointerID*/);
		}
	}
}

static void OnCursorPos(GLFWwindow *window, double xpos, double ypos)
{
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if (auto inputSink = self->GetInputSink())
		{
			vec2d pxMousePos = GetCursorPosInPixels(window, xpos, ypos);
			inputSink->OnPointer(UI::PointerType::Mouse, UI::Msg::PointerMove, pxMousePos, vec2d{}/*offset*/, 0 /*buttons*/, 0 /*pointerID*/);
		}
	}
}

static void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if (auto inputSink = self->GetInputSink())
		{
			vec2d pxMousePos = GetCursorPosInPixels(window);
			vec2d pxMouseOffset = GetCursorPosInPixels(window, xoffset, yoffset);
			inputSink->OnPointer(UI::PointerType::Mouse, UI::Msg::Scroll, pxMousePos, pxMouseOffset, 0 /*buttons*/, 0 /*pointerID*/);
		}
	}
}

static void OnKey(GLFWwindow *window, int platformKey, int scancode, int platformAction, int mods)
{
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if (auto inputSink = self->GetInputSink())
		{
			UI::Key key = MapGlfwKeyCode(platformKey);
			UI::Msg action = (GLFW_RELEASE == platformAction) ? UI::Msg::KeyReleased : UI::Msg::KeyPressed;
			inputSink->OnKey(key, action);
		}
	}
}

static void OnChar(GLFWwindow *window, unsigned int codepoint)
{
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if( codepoint < 57344 || codepoint > 63743 ) // ignore Private Use Area characters
		{
			if (auto inputSink = self->GetInputSink())
			{
				inputSink->OnChar(codepoint);
			}
		}
	}
}

static void OnRefresh(GLFWwindow *window)
{
	if (auto self = (GlfwAppWindow *)glfwGetWindowUserPointer(window))
	{
		if (auto inputSink = self->GetInputSink())
		{
			inputSink->OnRefresh();
		}
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

UI::IClipboard& GlfwAppWindow::GetClipboard()
{
	return *_clipboard;
}

UI::IInput& GlfwAppWindow::GetInput()
{
	return *_input;
}

IRender& GlfwAppWindow::GetRender()
{
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

void GlfwAppWindow::SetMouseCursor(MouseCursor mouseCursor)
{
	switch (mouseCursor)
	{
	case MouseCursor::Arrow:
		glfwSetCursor(_window.get(), _cursorArrow.get());
		break;
	case MouseCursor::IBeam:
		glfwSetCursor(_window.get(), _cursorIBeam.get());
		break;
	default:
		glfwSetCursor(_window.get(), nullptr);
		break;
	}
}

void GlfwAppWindow::MakeCurrent()
{
	glfwMakeContextCurrent(_window.get());
}

void GlfwAppWindow::Present()
{
	assert(glfwGetCurrentContext() == _window.get());
	glfwSwapBuffers(_window.get());
}

bool GlfwAppWindow::ShouldClose() const
{
	return !!glfwWindowShouldClose(_window.get());
}

void GlfwAppWindow::PollEvents() // static
{
	glfwPollEvents();
}

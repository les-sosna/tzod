#include "inc/platglfw/GlfwAppWindow.h"
#include "inc/platglfw/GlfwPlatform.h"
#include "inc/platglfw/GlfwKeys.h"
#include <ui/InputContext.h>
#include <ui/GuiManager.h>
#include <ui/LayoutContext.h>
#include <ui/Pointers.h>
#include <ui/StateContext.h>
#include <ui/Window.h>
#include <video/RenderOpenGL.h>
#include <GLFW/glfw3.h>
#include <stdexcept>

GlfwInitHelper::GlfwInitHelper()
{
	if( !glfwInit() )
		throw std::runtime_error("Failed to initialize OpenGL");
}

GlfwInitHelper::~GlfwInitHelper()
{
	glfwTerminate();
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

static vec2d GetPixelSize(GLFWwindow *window)
{
	int width;
	int height;
	glfwGetFramebufferSize(window, &width, &height);
	return vec2d{static_cast<float>(width), static_cast<float>(height)};
}

static void OnMouseButton(GLFWwindow *window, int button, int action, int mods)
try
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		UI::Msg msg = (GLFW_RELEASE == action) ? UI::Msg::PointerUp : UI::Msg::PointerDown;
		int buttons = 0;
		switch (button)
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

		gui->GetInputContext().ProcessPointer(
			gui->GetTextureManager(),
			gui->GetDesktop(),
			UI::LayoutContext(GetLayoutScale(window), vec2d{}, GetPixelSize(window), gui->GetDesktop()->GetEnabled()),
			UI::StateContext(),
			pxMousePos,
			vec2d{},
			msg,
			buttons,
			UI::PointerType::Mouse,
			0);
	}
}
catch (const std::exception &e)
{

}

static void OnCursorPos(GLFWwindow *window, double xpos, double ypos)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		vec2d pxMousePos = GetCursorPosInPixels(window, xpos, ypos);
		gui->GetInputContext().ProcessPointer(
			gui->GetTextureManager(),
			gui->GetDesktop(),
			UI::LayoutContext(GetLayoutScale(window), vec2d{}, GetPixelSize(window), gui->GetDesktop()->GetEnabled()),
			UI::StateContext(),
			pxMousePos,
			vec2d{},
			UI::Msg::PointerMove,
			0,
			UI::PointerType::Mouse,
			0);
	}
}

static void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		vec2d pxMousePos = GetCursorPosInPixels(window);
		vec2d pxMouseOffset = GetCursorPosInPixels(window, xoffset, yoffset);
		gui->GetInputContext().ProcessPointer(
			gui->GetTextureManager(),
			gui->GetDesktop(),
			UI::LayoutContext(GetLayoutScale(window), vec2d{}, GetPixelSize(window), gui->GetDesktop()->GetEnabled()),
			UI::StateContext(),
			pxMousePos,
			pxMouseOffset,
			UI::Msg::Scroll,
			0,
			UI::PointerType::Mouse,
			0);
	}
}

static void OnKey(GLFWwindow *window, int platformKey, int scancode, int action, int mods)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		UI::Key key = MapGlfwKeyCode(platformKey);
		gui->GetInputContext().ProcessKeys(gui->GetDesktop(), GLFW_RELEASE == action ? UI::Msg::KEYUP : UI::Msg::KEYDOWN, key);
	}
}

static void OnChar(GLFWwindow *window, unsigned int codepoint)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		if( codepoint < 57344 || codepoint > 63743 ) // ignore Private Use Area characters
		{
			gui->GetInputContext().ProcessText(gui->GetDesktop(), codepoint);
		}
	}
}

static void OnFramebufferSize(GLFWwindow *window, int width, int height)
{
	if (auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window))
	{
		float layoutScale = GetLayoutScale(window);
		gui->GetDesktop()->Resize((float)width / layoutScale, (float)height / layoutScale);
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
	, _clipboard(new GlfwClipboard(*_window))
	, _input(new GlfwInput(*_window))
	, _render(RenderCreateOpenGL())
{
	glfwSetMouseButtonCallback(_window.get(), OnMouseButton);
	glfwSetCursorPosCallback(_window.get(), OnCursorPos);
	glfwSetScrollCallback(_window.get(), OnScroll);
	glfwSetKeyCallback(_window.get(), OnKey);
	glfwSetCharCallback(_window.get(), OnChar);
	glfwSetFramebufferSizeCallback(_window.get(), OnFramebufferSize);

	glfwMakeContextCurrent(_window.get());
	glfwSwapInterval(1);
}

GlfwAppWindow::~GlfwAppWindow()
{
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

unsigned int GlfwAppWindow::GetPixelWidth()
{
	int width;
	glfwGetFramebufferSize(_window.get(), &width, nullptr);
	return width;
}

unsigned int GlfwAppWindow::GetPixelHeight()
{
	int height;
	glfwGetFramebufferSize(_window.get(), nullptr, &height);
	return height;
}

float GlfwAppWindow::GetLayoutScale()
{
	return ::GetLayoutScale(_window.get());
}

void GlfwAppWindow::SetInputSink(UI::LayoutManager *inputSink)
{
	glfwSetWindowUserPointer(_window.get(), inputSink);
}

void GlfwAppWindow::MakeCurrent()
{
	glfwMakeContextCurrent(_window.get());
}

void GlfwAppWindow::Present()
{
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

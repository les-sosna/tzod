#include "inc/platglfw/GlfwAppWindow.h"
#include "inc/platglfw/GlfwPlatform.h"
#include "inc/platglfw/GlfwKeys.h"
#include <ui/GuiManager.h>
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

static void OnMouseButton(GLFWwindow *window, int button, int action, int mods)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		UI::Msg msg;
		switch (button)
		{
			case GLFW_MOUSE_BUTTON_LEFT:
				msg = (GLFW_RELEASE == action) ? UI::MSGLBUTTONUP : UI::MSGLBUTTONDOWN;
				break;
			case GLFW_MOUSE_BUTTON_RIGHT:
				msg = (GLFW_RELEASE == action) ? UI::MSGRBUTTONUP : UI::MSGRBUTTONDOWN;
				break;
			case GLFW_MOUSE_BUTTON_MIDDLE:
				msg = (GLFW_RELEASE == action) ? UI::MSGMBUTTONUP : UI::MSGMBUTTONDOWN;
				break;
			default:
				return;
		}
		double xpos = 0;
		double ypos = 0;
		glfwGetCursorPos(window, &xpos, &ypos);
		gui->ProcessMouse((float) xpos, (float) ypos, 0, msg);
	}
}

static void OnCursorPos(GLFWwindow *window, double xpos, double ypos)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		gui->ProcessMouse((float) xpos, (float) ypos, 0, UI::MSGMOUSEMOVE);
	}
}

static void OnScroll(GLFWwindow *window, double xoffset, double yoffset)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		double xpos = 0;
		double ypos = 0;
		glfwGetCursorPos(window, &xpos, &ypos);
		gui->ProcessMouse((float) xpos, (float) ypos, (float) yoffset, UI::MSGMOUSEWHEEL);
	}
}

static void OnKey(GLFWwindow *window, int platformKey, int scancode, int action, int mods)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		UI::Key key = MapGlfwKeyCode(platformKey);
		gui->ProcessKeys(GLFW_RELEASE == action ? UI::MSGKEYUP : UI::MSGKEYDOWN, key);
	}
}

static void OnChar(GLFWwindow *window, unsigned int codepoint)
{
	if( auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window) )
	{
		if( codepoint < 57344 || codepoint > 63743 ) // ignore Private Use Area characters
		{
			gui->ProcessText(codepoint);
		}
	}
}

static void OnFramebufferSize(GLFWwindow *window, int width, int height)
{
	if (auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window))
	{
		gui->GetDesktop()->Resize((float)width, (float)height);
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

void GlfwAppWindow::SetInputSink(UI::LayoutManager *inputSink)
{
	glfwSetWindowUserPointer(_window.get(), inputSink);
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

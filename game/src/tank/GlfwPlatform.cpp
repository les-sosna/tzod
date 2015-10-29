#include "GlfwPlatform.h"
#include "GlfwKeys.h"
#include <ui/GuiManager.h>
#include <GLFW/glfw3.h>

GlfwInput::GlfwInput(GLFWwindow &window)
	: _window(window)
{}

bool GlfwInput::IsKeyPressed(UI::Key key) const
{
	int platformKey = UnmapGlfwKeyCode(key);
	return GLFW_PRESS == glfwGetKey(&_window, platformKey);
}

bool GlfwInput::IsMousePressed(int button) const
{
	return GLFW_PRESS == glfwGetMouseButton(&_window, button);
}

vec2d GlfwInput::GetMousePos() const
{
	double x, y;
	glfwGetCursorPos(&_window, &x, &y);
	return vec2d((float) x, (float) y);
}


GlfwClipboard::GlfwClipboard(GLFWwindow &window)
	: _window(window)
{}

const char* GlfwClipboard::GetClipboardText() const
{
	return glfwGetClipboardString(&_window);
}

void GlfwClipboard::SetClipboardText(std::string text)
{
	glfwSetClipboardString(&_window, text.c_str());
}


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
    auto gui = (UI::LayoutManager *) glfwGetWindowUserPointer(window);
    gui->GetDesktop()->Resize((float) width, (float) height);
}

GlfwAppWindow::GlfwAppWindow(const char *title, bool fullscreen, int width, int height)
	: _window(glfwCreateWindow(fullscreen ? glfwGetVideoMode(glfwGetPrimaryMonitor())->width : width,
	                           fullscreen ? glfwGetVideoMode(glfwGetPrimaryMonitor())->height : height,
	                           title,
	                           fullscreen ? glfwGetPrimaryMonitor() : nullptr,
	                           nullptr))
{
	if (!_window)
		throw std::runtime_error("Failed to create GLFW window");

	glfwSetMouseButtonCallback(_window.get(), OnMouseButton);
	glfwSetCursorPosCallback(_window.get(), OnCursorPos);
	glfwSetScrollCallback(_window.get(), OnScroll);
	glfwSetKeyCallback(_window.get(), OnKey);
	glfwSetCharCallback(_window.get(), OnChar);
	glfwSetFramebufferSizeCallback(_window.get(), OnFramebufferSize);
}

GlfwAppWindow::~GlfwAppWindow()
{
}

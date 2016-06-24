#include "inc/platglfw/GlfwPlatform.h"
#include "inc/platglfw/GlfwKeys.h"
#include <GLFW/glfw3.h>

vec2d GetCursorPosInPixels(GLFWwindow *window, double dipX, double dipY)
{
	int pxWidth;
	int pxHeight;
	glfwGetFramebufferSize(window, &pxWidth, &pxHeight);

	int dipWidth;
	int dipHeight;
	glfwGetWindowSize(window, &dipWidth, &dipHeight);

	return{ float(dipX * pxWidth / dipWidth), float(dipY * pxHeight / dipHeight) };
}

vec2d GetCursorPosInPixels(GLFWwindow *window)
{
    double dipX = 0;
    double dipY = 0;
    glfwGetCursorPos(window, &dipX, &dipY);
    return GetCursorPosInPixels(window, dipX, dipY);
}


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
	return GLFW_PRESS == glfwGetMouseButton(&_window, button-1);
}

vec2d GlfwInput::GetMousePos() const
{
	return GetCursorPosInPixels(&_window);
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

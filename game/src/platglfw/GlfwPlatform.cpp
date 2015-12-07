#include "inc/platglfw/GlfwPlatform.h"
#include "inc/platglfw/GlfwKeys.h"
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

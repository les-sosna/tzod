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

bool GlfwInput::IsKeyPressed(Plat::Key key) const
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

Plat::GamepadState GlfwInput::GetGamepadState(unsigned int index) const
{
	Plat::GamepadState gamepadState = {};
	if (glfwJoystickPresent(GLFW_JOYSTICK_1 + index))
	{
		// XBox One/360 controller mapping

		int axesCount = 0;
		const float *axes = glfwGetJoystickAxes(GLFW_JOYSTICK_1 + index, &axesCount);
		if (axesCount >= 6)
		{
			gamepadState.leftThumbstickPos.x = axes[0];
			gamepadState.leftThumbstickPos.y = -axes[1];
			gamepadState.rightThumbstickPos.x = axes[2];
			gamepadState.rightThumbstickPos.y = -axes[3];
			gamepadState.leftTrigger = axes[4];
			gamepadState.rightTrigger = axes[5];
		}

		int buttonsCount = 0;
		const unsigned char *buttons = glfwGetJoystickButtons(GLFW_JOYSTICK_1 + index, &buttonsCount);
		if (buttonsCount >= 14)
		{
			gamepadState.A = !!buttons[0];
			gamepadState.B = !!buttons[1];
			gamepadState.X = !!buttons[2];
			gamepadState.Y = !!buttons[3];
			gamepadState.leftShoulder = !!buttons[4];
			gamepadState.rightShoulder = !!buttons[5];
			gamepadState.view = !!buttons[6];
			gamepadState.menu = !!buttons[7];
			gamepadState.leftThumbstickPressed = !!buttons[8];
			gamepadState.rightThumbstickPressed = !!buttons[9];
			gamepadState.DPadUp = !!buttons[10];
			gamepadState.DPadRight = !!buttons[11];
			gamepadState.DPadDown = !!buttons[12];
			gamepadState.DPadLeft = !!buttons[13];
		}
	}
	return gamepadState;
}

bool GlfwInput::GetSystemNavigationBackAvailable() const
{
	return true;
}


GlfwClipboard::GlfwClipboard(GLFWwindow &window)
	: _window(window)
{}

std::string_view GlfwClipboard::GetClipboardText() const
{
	const char *text = glfwGetClipboardString(&_window);
	return text ? text : std::string_view();
}

void GlfwClipboard::SetClipboardText(std::string text)
{
	glfwSetClipboardString(&_window, text.c_str());
}

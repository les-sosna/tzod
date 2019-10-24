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

Plat::PointerState GlfwInput::GetPointerState(unsigned int index) const
{
	Plat::PointerState result = { Plat::PointerType::Unknown };
	if (index == 0 && _mousePresent)
	{
		result.type = Plat::PointerType::Mouse;
		result.button1 = GLFW_PRESS == glfwGetMouseButton(&_window, 0);
		result.button2 = GLFW_PRESS == glfwGetMouseButton(&_window, 1);
		result.button3 = GLFW_PRESS == glfwGetMouseButton(&_window, 2);
		result.pressed = result.button1 || result.button2 || result.button3;
		result.position = GetCursorPosInPixels(&_window);
	}
	return result;
}

Plat::GamepadState GlfwInput::GetGamepadState(unsigned int index) const
{
	Plat::GamepadState gamepadState = {};
	GLFWgamepadstate glfwState;
	if (glfwGetGamepadState(GLFW_JOYSTICK_1 + index, &glfwState))
	{
		gamepadState.leftTrigger = glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_TRIGGER];
		gamepadState.rightTrigger = glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_TRIGGER];
		gamepadState.leftThumbstickPos.x = glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_X];
		gamepadState.leftThumbstickPos.y = glfwState.axes[GLFW_GAMEPAD_AXIS_LEFT_Y];
		gamepadState.rightThumbstickPos.x = glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_X];
		gamepadState.rightThumbstickPos.y = glfwState.axes[GLFW_GAMEPAD_AXIS_RIGHT_Y];

		gamepadState.A = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_A]);
		gamepadState.B = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_B]);
		gamepadState.X = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_X]);
		gamepadState.Y = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_Y]);
		gamepadState.leftShoulder = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_LEFT_BUMPER]);
		gamepadState.rightShoulder = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER]);
		gamepadState.view = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_BACK]);
		gamepadState.menu = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_START]);
		gamepadState.leftThumbstickPressed = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_LEFT_THUMB]);
		gamepadState.rightThumbstickPressed = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_RIGHT_THUMB]);
		gamepadState.DPadUp = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_UP]);
		gamepadState.DPadRight = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_RIGHT]);
		gamepadState.DPadDown = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_DOWN]);
		gamepadState.DPadLeft = (GLFW_PRESS == glfwState.buttons[GLFW_GAMEPAD_BUTTON_DPAD_LEFT]);
	}
	return gamepadState;
}

bool GlfwInput::GetSystemNavigationBackAvailable() const
{
	return true; // keyboard Esc should be available
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

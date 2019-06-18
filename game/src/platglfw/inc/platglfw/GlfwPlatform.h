#pragma once
#include <plat/Clipboard.h>
#include <plat/Input.h>

struct GLFWwindow;

class GlfwInput final
	: public Plat::Input
{
public:
	GlfwInput(GLFWwindow &window);
	void SetMousePresent(bool mousePresent) { _mousePresent = mousePresent; }

	// Plat::Input
	bool IsKeyPressed(Plat::Key key) const override;
	Plat::PointerState GetPointerState(unsigned int index) const override;
	Plat::GamepadState GetGamepadState(unsigned int index) const override;
	bool GetSystemNavigationBackAvailable() const override;

private:
	GLFWwindow &_window;
	bool _mousePresent = false;
};

class GlfwClipboard final
	: public Plat::Clipboard
{
public:
	GlfwClipboard(GLFWwindow &window);

	// Plat::Clipboard
	std::string_view GetClipboardText() const override;
	void SetClipboardText(std::string text) override;

private:
	GLFWwindow &_window;
};

vec2d GetCursorPosInPixels(GLFWwindow *window);
vec2d GetCursorPosInPixels(GLFWwindow *window, double dipX, double dipY);

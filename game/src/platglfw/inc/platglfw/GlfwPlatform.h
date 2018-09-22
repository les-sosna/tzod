#pragma once
#include <ui/UIInput.h>
#include <plat/Clipboard.h>

struct GLFWwindow;

class GlfwInput : public UI::IInput
{
public:
	GlfwInput(GLFWwindow &window);

	// UI::IInput
	bool IsKeyPressed(UI::Key key) const override;
	bool IsMousePressed(int button) const override;
	vec2d GetMousePos() const override;
	UI::GamepadState GetGamepadState(unsigned int index) const override;
	bool GetSystemNavigationBackAvailable() const override;

private:
	GLFWwindow &_window;
};

class GlfwClipboard : public Clipboard
{
public:
	GlfwClipboard(GLFWwindow &window);

	// Clipboard
	std::string_view GetClipboardText() const override;
	void SetClipboardText(std::string text) override;

private:
	GLFWwindow &_window;
};

vec2d GetCursorPosInPixels(GLFWwindow *window);
vec2d GetCursorPosInPixels(GLFWwindow *window, double dipX, double dipY);

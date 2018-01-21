#pragma once
#include <plat/AppWindow.h>
#include <memory>

class GlfwClipboard;
class GlfwInput;
struct GLFWcursor;
struct GLFWwindow;

struct GlfwCursorDeleter
{
	void operator()(GLFWcursor *cursor);
};

struct GlfwWindowDeleter
{
	void operator()(GLFWwindow *window);
};

class GlfwInitHelper
{
public:
	GlfwInitHelper();
	~GlfwInitHelper();

	GlfwInitHelper(GlfwInitHelper &&other);

private:
	bool _isActive;
	static unsigned int s_initCount;
};

class GlfwAppWindow : public AppWindow
{
public:
	GlfwAppWindow(const char *title, bool fullscreen, int width, int height);
	~GlfwAppWindow();

	static void PollEvents();

	bool ShouldClose() const;

	// AppWindow
	AppWindowInputSink* GetInputSink() const override { return _inputSink; }
	void SetInputSink(AppWindowInputSink *inputSink) override { _inputSink = inputSink; }
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
	UI::IClipboard& GetClipboard() override;
	UI::IInput& GetInput() override;
	IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override;
	void SetMouseCursor(MouseCursor mouseCursor) override;
	void Present() override;
	void MakeCurrent() override;

private:
	GlfwInitHelper _initHelper;
	AppWindowInputSink *_inputSink = nullptr;
	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> _window;
	std::unique_ptr<GLFWcursor, GlfwCursorDeleter> _cursorArrow;
	std::unique_ptr<GLFWcursor, GlfwCursorDeleter> _cursorIBeam;
	std::unique_ptr<GlfwClipboard> _clipboard;
	std::unique_ptr<GlfwInput> _input;
	std::unique_ptr<IRender> _render;
};

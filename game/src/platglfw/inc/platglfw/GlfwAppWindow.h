#pragma once
#include <ui/AppWindow.h>
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

struct GlfwInitHelper
{
	GlfwInitHelper();
	~GlfwInitHelper();
};

class GlfwAppWindow : public AppWindow
{
public:
	GlfwAppWindow(const char *title, bool fullscreen, int width, int height);
	~GlfwAppWindow();

	static void PollEvents();
	void Present();
	bool ShouldClose() const;

	float GetPixelWidth() const;
	float GetPixelHeight() const;
	float GetLayoutScale() const;

	// AppWindow
	UI::IClipboard& GetClipboard() override;
	UI::IInput& GetInput() override;
	IRender& GetRender() override;
	void SetInputSink(UI::LayoutManager *inputSink) override;
	void SetMouseCursor(MouseCursor mouseCursor) override;
	void MakeCurrent() override;

private:
	GlfwInitHelper _initHelper;
	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> _window;
	std::unique_ptr<GLFWcursor, GlfwCursorDeleter> _cursorArrow;
	std::unique_ptr<GLFWcursor, GlfwCursorDeleter> _cursorIBeam;
	std::unique_ptr<GlfwClipboard> _clipboard;
	std::unique_ptr<GlfwInput> _input;
	std::unique_ptr<IRender> _render;
};

#pragma once
#include <app/AppWindow.h>
#include <memory>

class GlfwClipboard;
class GlfwInput;
struct GLFWwindow;

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

	// AppWindow
	UI::IClipboard& GetClipboard() override;
	UI::IInput& GetInput() override;
	unsigned int GetPixelWidth() override;
	unsigned int GetPixelHeight() override;
	void SetInputSink(UI::LayoutManager *inputSink) override;

private:
	GlfwInitHelper _initHelper;
	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> _window;
	std::unique_ptr<GlfwClipboard> _clipboard;
	std::unique_ptr<GlfwInput> _input;
};
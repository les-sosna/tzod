#pragma once
#include "GlfwPlatform.h"
#include <plat/AppWindow.h>
#include <memory>

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

class GlfwInitHelper final
{
public:
	GlfwInitHelper();
	~GlfwInitHelper();

	GlfwInitHelper(GlfwInitHelper &&other) noexcept;

private:
	bool _isActive;
	static unsigned int s_initCount;
};

class GlfwAppWindow final
	: public Plat::AppWindow
	, private Plat::AppWindowCommandClose
{
public:
	GlfwAppWindow(const char *title, bool fullscreen, int width, int height);
	~GlfwAppWindow();

	static void PollEvents(Plat::AppWindowInputSink& inputSink);

	bool ShouldClose() const;
	int _windowedTop = 0;
	int _windowedLeft = 0;
	int _windowedWidth = 0;
	int _windowedHeight = 0;

	// AppWindow
	int GetDisplayRotation() const override { return 0; }
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
	Plat::Clipboard& GetClipboard() override;
	GlfwInput& GetInput() override;
	IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override;
	void SetMouseCursor(Plat::MouseCursor mouseCursor) override;
	void Present() override;
	Plat::AppWindowCommandClose* CmdClose() override { return this; }

	// AppWindowCommandClose
	void RequestClose() override;

private:
	GlfwInitHelper _initHelper;
	std::unique_ptr<GLFWwindow, GlfwWindowDeleter> _window;
	std::unique_ptr<GLFWcursor, GlfwCursorDeleter> _cursorArrow;
	std::unique_ptr<GLFWcursor, GlfwCursorDeleter> _cursorIBeam;
	std::unique_ptr<GlfwClipboard> _clipboard;
	std::unique_ptr<GlfwInput> _input;
	std::unique_ptr<IRender> _render;
};

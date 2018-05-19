#include <plat/AppWindow.h>
#include <memory>

struct IRender;
@class GLKView;
@class UITapGestureRecognizer;

@class TouchAdapter;

class CocoaTouchWindow : public AppWindow
{
public:
    explicit CocoaTouchWindow(GLKView *view);
    ~CocoaTouchWindow();
    
    void SetSizeAndScale(float width, float height, float scale);

    // AppWindow
	AppWindowInputSink* GetInputSink() const override { return _inputSink; }
	void SetInputSink(AppWindowInputSink *inputSink) override { _inputSink = inputSink; }
	int GetDisplayRotation() const override;
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
    UI::IClipboard& GetClipboard() override;
    UI::IInput& GetInput() override;
    IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override {}
	void SetMouseCursor(MouseCursor mouseCursor) override {}
    void MakeCurrent() override {}
	void Present() override {}

private:
    GLKView *_glkView;
    std::unique_ptr<IRender> _render;
	AppWindowInputSink *_inputSink = nullptr;
    float _pxWidth = 0;
    float _pxHeight = 0;
    float _scale = 1;
};

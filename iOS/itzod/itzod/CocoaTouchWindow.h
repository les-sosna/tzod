#include <plat/AppWindow.h>
#include <memory>

struct IRender;
@class GLKView;
@class UITapGestureRecognizer;

@class TouchAdapter;

class CocoaTouchWindow : public Plat::AppWindow
{
public:
    explicit CocoaTouchWindow(GLKView *view);
    ~CocoaTouchWindow();
    
    void SetSizeAndScale(float width, float height, float scale);

    // AppWindow
	Plat::AppWindowInputSink* GetInputSink() const override { return _inputSink; }
	void SetInputSink(Plat::AppWindowInputSink *inputSink) override { _inputSink = inputSink; }
	int GetDisplayRotation() const override;
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
    Plat::Clipboard& GetClipboard() override;
    Plat::Input& GetInput() override;
    IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override {}
	void SetMouseCursor(Plat::MouseCursor mouseCursor) override {}
    void MakeCurrent() override {}
	void Present() override {}

private:
    GLKView *_glkView;
    std::unique_ptr<IRender> _render;
	Plat::AppWindowInputSink *_inputSink = nullptr;
    float _pxWidth = 0;
    float _pxHeight = 0;
    float _scale = 1;
};

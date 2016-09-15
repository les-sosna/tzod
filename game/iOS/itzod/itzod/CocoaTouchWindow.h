#include <ui/AppWindow.h>
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
    
    UI::LayoutManager* GetInputSink() const { return _inputSink; }
    void SetSizeAndScale(float width, float height, float scale);
    
    float GetPixelWidth() const;
    float GetPixelHeight() const;
    float GetLayoutScale() const;

    // AppWindow
    UI::IClipboard& GetClipboard() override;
    UI::IInput& GetInput() override;
    IRender& GetRender() override;
    void SetInputSink(UI::LayoutManager *inputSink) override;
    void MakeCurrent() override {}

private:
    GLKView *_glkView;
    std::unique_ptr<IRender> _render;
    UI::LayoutManager *_inputSink = nullptr;
    float _pxWidth = 0;
    float _pxHeight = 0;
    float _scale = 1;
};

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
    
    // AppWindow
    UI::IClipboard& GetClipboard() override;
    UI::IInput& GetInput() override;
    IRender& GetRender() override;
    unsigned int GetPixelWidth() override;
    unsigned int GetPixelHeight() override;
    float GetLayoutScale() override;
    void SetInputSink(UI::LayoutManager *inputSink) override;
    void MakeCurrent() override {}

private:
    GLKView *_glkView;
    std::unique_ptr<IRender> _render;
    UI::LayoutManager *_inputSink = nullptr;
    float _width = 110.f;
    float _height = 110.f;
    float _scale = 1.f;
};

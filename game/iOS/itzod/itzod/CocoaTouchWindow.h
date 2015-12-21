#include <app/AppWindow.h>
#include <memory>

struct IRender;

class CocoaTouchWindow : public AppWindow
{
public:
    CocoaTouchWindow();
    ~CocoaTouchWindow();
    
    void SetPixelSize(unsigned int width, unsigned int height);
    
    // AppWindow
    UI::IClipboard& GetClipboard() override;
    UI::IInput& GetInput() override;
    IRender& GetRender() override;
    unsigned int GetPixelWidth() override;
    unsigned int GetPixelHeight() override;
    void SetInputSink(UI::LayoutManager *inputSink) override;

private:
    std::unique_ptr<IRender> _render;
    UI::LayoutManager *_inputSink;
    unsigned int _width;
    unsigned int _height;
};

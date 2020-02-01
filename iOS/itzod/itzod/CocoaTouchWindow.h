#include <plat/AppWindow.h>
#include <video/RenderBinding.h>
#include <memory>

struct IRender;

class CocoaTouchWindow : public Plat::AppWindow
{
public:
    CocoaTouchWindow();
    ~CocoaTouchWindow();
    
    void SetSizeAndScale(float width, float height, float scale);
    IRender& GetRender();
    RenderBinding& GetRenderBinding() { return _renderBinding; }

    // AppWindow
	int GetDisplayRotation() const override;
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
    Plat::Clipboard& GetClipboard() override;
    Plat::Input& GetInput() override;
	void SetCanNavigateBack(bool canNavigateBack) override {}
	void SetMouseCursor(Plat::MouseCursor mouseCursor) override {}

private:
    std::unique_ptr<IRender> _render;
    RenderBinding _renderBinding;
    float _pxWidth = 0;
    float _pxHeight = 0;
    float _scale = 1;
};

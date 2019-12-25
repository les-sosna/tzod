#include <plat/AppWindow.h>
#include <memory>

struct IRender;

class CocoaTouchWindow : public Plat::AppWindow
{
public:
    CocoaTouchWindow();
    ~CocoaTouchWindow();
    
    void SetSizeAndScale(float width, float height, float scale);

    // AppWindow
	int GetDisplayRotation() const override;
	vec2d GetPixelSize() const override;
	float GetLayoutScale() const override;
    Plat::Clipboard& GetClipboard() override;
    Plat::Input& GetInput() override;
    IRender& GetRender() override;
	void SetCanNavigateBack(bool canNavigateBack) override {}
	void SetMouseCursor(Plat::MouseCursor mouseCursor) override {}
	void Present() override {}

private:
    std::unique_ptr<IRender> _render;
    float _pxWidth = 0;
    float _pxHeight = 0;
    float _scale = 1;
};

#include <ui/ScrollView.h>
#include <ui/DataContext.h>
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <gtest/gtest.h>

TEST(ScrollView, MeasureContent)
{
    TextureManager texman(nullptr);
    UI::ScrollView scrollView{};
    UI::DataContext dc{};
    float scale = 3;
    UI::LayoutConstraints constraints{};
    constraints.maxPixelSize = vec2d{ 7, 9 } * scale;
    
    auto noContent = scrollView.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(noContent == vec2d{});
    
    auto content = std::make_shared<UI::Window>();
    scrollView.SetContent(content);
    
    auto zeroSizeContent = scrollView.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(zeroSizeContent == vec2d{});
    
    content->Resize(7, 9);
    auto maxSizeContent = scrollView.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(maxSizeContent == constraints.maxPixelSize);

    content->Resize(1001, 1002);
    auto oversizeSizeContent = scrollView.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE((oversizeSizeContent == vec2d{1001 * scale, constraints.maxPixelSize.y}));
}

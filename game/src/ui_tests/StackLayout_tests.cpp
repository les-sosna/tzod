#include <ui/StackLayout.h>
#include <ui/DataContext.h>
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <gtest/gtest.h>

TEST(StackLayout, MeasureContent)
{
    TextureManager texman(nullptr);
    UI::StackLayout stackLayout;
    UI::DataContext dc;
    float scale = 3;
    UI::LayoutConstraints constraints{};
    constraints.maxPixelSize = vec2d{ 7, 9 } * scale;
    
    auto noContent = stackLayout.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(noContent == vec2d{});
    
    auto content1 = std::make_shared<UI::Window>();
    auto content2 = std::make_shared<UI::Window>();
    stackLayout.AddFront(content1);
    stackLayout.AddFront(content2);

    auto zeroSizeContent = stackLayout.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(zeroSizeContent == vec2d{});
    
    content1->Resize(42, 43);
    content2->Resize(111, 112);
    
    auto overSizeContent = stackLayout.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE((overSizeContent == vec2d{ 111, 43 + 112 } * scale));
}

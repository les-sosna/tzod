#include <ui/StackLayout.h>
#include <ui/DataContext.h>
#include <ui/LayoutContext.h>
#include <video/TextureManager.h>
#include <gtest/gtest.h>

TEST(StackLayout, MeasureContent)
{
    TextureManager texman;
    UI::StackLayout stackLayout;
    UI::DataContext dc;
    float scale = 3;
    UI::LayoutConstraints constraints{};
    constraints.maxPixelSize = vec2d{ 7, 9 } * scale;

    auto noContent = stackLayout.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(noContent == vec2d{});
    
    auto child1 = std::make_shared<UI::Window>();
    auto child2 = std::make_shared<UI::Window>();
    stackLayout.AddFront(child1);
    stackLayout.AddFront(child2);

    auto zeroSizeContent = stackLayout.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE(zeroSizeContent == vec2d{});
    
    child1->Resize(42, 43);
    child2->Resize(111, 112);
    
    auto overSizeContent = stackLayout.GetContentSize(texman, dc, scale, constraints);
    EXPECT_TRUE((overSizeContent == vec2d{ 111, 43 + 112 } * scale));
}

TEST(StackLayout, ChildrenLayout)
{
    TextureManager texman;
    UI::StackLayout stackLayout;
    stackLayout.SetAlign(UI::Align::CT);

    auto child1 = std::make_shared<UI::Window>();
    auto child2 = std::make_shared<UI::Window>();
    auto child3 = std::make_shared<UI::Window>();
    child1->Resize(1, 0); // 3 px
    child2->Resize(2, 0); // 6 px
    child3->Resize(3, 0); // 9 px
    stackLayout.AddFront(child1);
    stackLayout.AddFront(child2);
    stackLayout.AddFront(child3);

    float opacity = 1;
    float scale = 3;
    vec2d pxOffset = { -10, -20 };
    vec2d pxSize = { 6, 0 };
    bool enabled = true;
    bool focused = true;
    UI::LayoutContext lc(opacity, scale, pxOffset, pxSize, enabled, focused);
    UI::DataContext dc;

    {
        // child width is less than layout
        auto childLayout1 = stackLayout.GetChildLayout(texman, lc, dc, *child1);
        EXPECT_EQ(child1->GetWidth() * scale, WIDTH(childLayout1.rect)); // same size
        EXPECT_EQ(1, childLayout1.rect.left); // rounded left
    }
    {
        // child width is greater than layout
        auto childLayout2 = stackLayout.GetChildLayout(texman, lc, dc, *child2);
        EXPECT_EQ(child2->GetWidth() * scale, WIDTH(childLayout2.rect)); // same size
        EXPECT_EQ(0, childLayout2.rect.left);
    }
    {
        // child width is greater than layout
        auto childLayout3 = stackLayout.GetChildLayout(texman, lc, dc, *child3);
        EXPECT_EQ(child3->GetWidth() * scale, WIDTH(childLayout3.rect)); // same size
        EXPECT_EQ(-2, childLayout3.rect.left); // rounded left
    }
}

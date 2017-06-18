#import "GameView.h"
#include "CocoaTouchWindow.h"
#include <ui/GuiManager.h>
#include <ui/InputContext.h>
#include <ui/LayoutContext.h>
#include <ui/Pointers.h>
#include <ui/DataContext.h>
#include <map>

@interface GameView ()
{
    std::shared_ptr<CocoaTouchWindow> _appWindow;
    std::map<UITouch*, int> _touches;
}

@property (nonatomic) UITapGestureRecognizer* singleFingerTap;
@property (nonatomic) UIPanGestureRecognizer* singleFingerPan;

@end

static int GetFreeTouchIndex(const std::map<UITouch*, int> &touches)
{
    int index = 0;
    for (; index <= (int) touches.size(); ++index)
    {
        bool inUse = false;
        for (auto &pair: touches)
        {
            if (pair.second == index)
            {
                inUse = true;
                break;
            }
        }
        if (!inUse)
        {
            break;
        }
    }
    return index;
}

static unsigned int GetPointerID(int touchIndex)
{
    return 100U + touchIndex;
}

@implementation GameView

- (void)handleSingleTap:(UITapGestureRecognizer *)recognizer
{
    CGPoint location = [recognizer locationInView: recognizer.view];
    
    if (UI::LayoutManager *sink = _appWindow->GetInputSink())
    {
        auto touchIndex = GetFreeTouchIndex(_touches);
        auto pointerID = GetPointerID(touchIndex);
        auto desktop = sink->GetDesktop();
        vec2d pxDesktopSize{_appWindow->GetPixelWidth(), _appWindow->GetPixelHeight()};
        vec2d pxPointerPos{
            static_cast<float>(location.x * self.contentScaleFactor),
            static_cast<float>(location.y * self.contentScaleFactor)};
        sink->GetInputContext().ProcessPointer(sink->GetTextureManager(),
                                               desktop,
                                               UI::LayoutContext(1.f, _appWindow->GetLayoutScale(), pxDesktopSize, true),
                                               UI::DataContext(),
                                               pxPointerPos,
                                               vec2d{},
                                               UI::Msg::TAP,
                                               0, // button
                                               UI::PointerType::Touch,
                                               pointerID);
    }
}

- (void)handlePan:(UIPanGestureRecognizer *)recognizer
{
    CGPoint location = [recognizer locationInView: recognizer.view];
    CGPoint translation = [recognizer translationInView: recognizer.view];
    [recognizer setTranslation:CGPointMake(0, 0) inView: recognizer.view];
    
    if (UI::LayoutManager *sink = _appWindow->GetInputSink())
    {
        auto touchIndex = GetFreeTouchIndex(_touches);
        auto pointerID = GetPointerID(touchIndex);
        auto desktop = sink->GetDesktop();
        vec2d pxDesktopSize{_appWindow->GetPixelWidth(), _appWindow->GetPixelHeight()};
        vec2d pxPointerPos{
            static_cast<float>(location.x * self.contentScaleFactor),
            static_cast<float>(location.y * self.contentScaleFactor)};
        sink->GetInputContext().ProcessPointer(sink->GetTextureManager(),
                                               desktop,
                                               UI::LayoutContext(1.f, _appWindow->GetLayoutScale(), pxDesktopSize, true),
                                               UI::DataContext(),
                                               pxPointerPos,
                                               vec2d{static_cast<float>(translation.x), static_cast<float>(translation.y)}/30 * self.contentScaleFactor / _appWindow->GetLayoutScale(),
                                               UI::Msg::Scroll,
                                               0, // button
                                               UI::PointerType::Touch,
                                               pointerID);
    }
}

- (CocoaTouchWindow&)appWindow
{
    return *_appWindow;
}

- (void)setContext:(EAGLContext *)context
{
    [super setContext:context];
    [EAGLContext setCurrentContext:context];
    _appWindow.reset(new CocoaTouchWindow(self));
}

- (void)layoutSubviews
{
    [super layoutSubviews];
    _appWindow->SetSizeAndScale(self.bounds.size.width, self.bounds.size.height, self.contentScaleFactor);
}


- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    
    // FIXME: tap recognizer causes active touch to be cancelled after the tap event is generated
    //        this causes buttons to fail - they ignore taps when a touch is active.
    //        events order: Begin-Tap-Cancel
    //        Buttons do not generate click event if the pointer event is cancelled
    // Consider: set cancelsTouchesInView=NO
    
    self.singleFingerTap = [[UITapGestureRecognizer alloc]
                            initWithTarget:self
                            action:@selector(handleSingleTap:)];
    self.singleFingerTap.cancelsTouchesInView = NO;
    [self addGestureRecognizer:self.singleFingerTap];
    
    self.singleFingerPan = [[UIPanGestureRecognizer alloc]
                            initWithTarget:self
                            action:@selector(handlePan:)];
    self.singleFingerPan.cancelsTouchesInView = NO;
    [self addGestureRecognizer:self.singleFingerPan];
    
    self.multipleTouchEnabled = YES;
    return self;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    for (UITouch *touch in touches)
    {
        auto touchIndex = GetFreeTouchIndex(_touches);
        auto pointerID = GetPointerID(touchIndex);
        assert(0 == _touches.count(touch));
        _touches[touch] = touchIndex;
     
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            auto desktop = sink->GetDesktop();
            vec2d pxDesktopSize{_appWindow->GetPixelWidth(), _appWindow->GetPixelHeight()};
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->GetInputContext().ProcessPointer(sink->GetTextureManager(),
                                                   desktop,
                                                   UI::LayoutContext(1.f, _appWindow->GetLayoutScale(), pxDesktopSize, true),
                                                   UI::DataContext(),
                                                   pxPointerPos,
                                                   vec2d{},
                                                   UI::Msg::PointerDown,
                                                   1, // button
                                                   UI::PointerType::Touch,
                                                   pointerID);
        }
    }
    [super touchesBegan: touches withEvent: event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    for (UITouch *touch in touches)
    {
        auto it = _touches.find(touch);
        
        // Workaround: we may miss touchesBegan due to gesture recognizers
        if (_touches.end() == it)
            continue;
            
        auto pointerID = GetPointerID(it->second);
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            auto desktop = sink->GetDesktop();
            vec2d pxDesktopSize{_appWindow->GetPixelWidth(), _appWindow->GetPixelHeight()};
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->GetInputContext().ProcessPointer(sink->GetTextureManager(),
                                                   desktop,
                                                   UI::LayoutContext(1.f, _appWindow->GetLayoutScale(), pxDesktopSize, true),
                                                   UI::DataContext(),
                                                   pxPointerPos,
                                                   vec2d{},
                                                   UI::Msg::PointerMove,
                                                   0, // button
                                                   UI::PointerType::Touch,
                                                   pointerID);
        }
    }
    
    [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    for (UITouch *touch in touches)
    {
        auto it = _touches.find(touch);

        // Workaround: we may miss touchesBegan due to gesture recognizers
        if (_touches.end() == it)
            continue;
        
        auto pointerID = GetPointerID(it->second);
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            auto desktop = sink->GetDesktop();
            vec2d pxDesktopSize{_appWindow->GetPixelWidth(), _appWindow->GetPixelHeight()};
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->GetInputContext().ProcessPointer(sink->GetTextureManager(),
                                                   desktop,
                                                   UI::LayoutContext(1.f, _appWindow->GetLayoutScale(), pxDesktopSize, true),
                                                   UI::DataContext(),
                                                   pxPointerPos,
                                                   vec2d{},
                                                   UI::Msg::PointerUp,
                                                   1, // button
                                                   UI::PointerType::Touch,
                                                   pointerID);
        }
        _touches.erase(it);
    }
    
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(nonnull NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    for (UITouch *touch in touches)
    {
        auto it = _touches.find(touch);
        
        // Workaround: we may miss touchesBegan due to gesture recognizers
        if (_touches.end() == it)
            continue;
        
        auto pointerID = GetPointerID(it->second);
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            auto desktop = sink->GetDesktop();
            vec2d pxDesktopSize{_appWindow->GetPixelWidth(), _appWindow->GetPixelHeight()};
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->GetInputContext().ProcessPointer(sink->GetTextureManager(),
                                                   desktop,
                                                   UI::LayoutContext(1.f, _appWindow->GetLayoutScale(), pxDesktopSize, true),
                                                   UI::DataContext(),
                                                   pxPointerPos,
                                                   vec2d{},
                                                   UI::Msg::PointerCancel,
                                                   0, // button
                                                   UI::PointerType::Touch,
                                                   pointerID);
        }
        _touches.erase(it);
    }
    
    [super touchesCancelled:touches withEvent:event];
}

- (void)touchesEstimatedPropertiesUpdated:(NSSet * _Nonnull)touches
{
    [super touchesEstimatedPropertiesUpdated: touches];
}

@end

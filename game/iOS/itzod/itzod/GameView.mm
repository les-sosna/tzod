#import "GameView.h"
#include "CocoaTouchWindow.h"
#include <ui/GuiManager.h>
#include <ui/Pointers.h>
#include <map>

@interface GameView ()
{
    std::shared_ptr<CocoaTouchWindow> _appWindow;
    std::map<UITouch*, int> _touches;
}

@property (nonatomic) UITapGestureRecognizer* singleFingerTap;

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
        sink->ProcessPointer(location.x * self.contentScaleFactor,
                             location.y * self.contentScaleFactor,
                             0, // z
                             UI::Msg::TAP,
                             0, // button
                             UI::PointerType::Touch,
                             pointerID);
    }
}

- (AppWindow&)appWindow
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
    _appWindow->SetPixelSize(self.bounds.size.width * self.contentScaleFactor,
                             self.bounds.size.height * self.contentScaleFactor);
}


- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
    
    // FIXME: tap recognizer causes active touch to be cancelled after the tap event is generated
    //        this causes buttons to fail - they ignore taps when a touch is active.
    //        events order: Begin-Tap-Cancel
    //        Buttons do not generate click event if the pointer event is cancelled
    // Consider: set cancelsTouchesInView=NO
    
//    self.singleFingerTap = [[UITapGestureRecognizer alloc]
//                            initWithTarget:self
//                            action:@selector(handleSingleTap:)];
//    [self addGestureRecognizer:self.singleFingerTap];
    
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
            sink->ProcessPointer(location.x * self.contentScaleFactor,
                                 location.y * self.contentScaleFactor,
                                 0, // z
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
        assert(_touches.count(touch));
        auto pointerID = GetPointerID(_touches[touch]);
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            sink->ProcessPointer(location.x * self.contentScaleFactor,
                                 location.y * self.contentScaleFactor,
                                 0, // z
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
        assert(it != _touches.end());
        auto pointerID = GetPointerID(it->second);
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            sink->ProcessPointer(location.x * self.contentScaleFactor,
                                 location.y * self.contentScaleFactor,
                                 0, // z
                                 UI::Msg::PointerUp,
                                 1, // button
                                 UI::PointerType::Touch,
                                 pointerID);
        }
        _touches.erase(it);
    }
    
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(nullable NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    for (UITouch *touch in touches)
    {
        auto it = _touches.find(touch);
        assert(it != _touches.end());
        auto pointerID = GetPointerID(it->second);
        if (UI::LayoutManager *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            sink->ProcessPointer(location.x * self.contentScaleFactor,
                                 location.y * self.contentScaleFactor,
                                 0, // z
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

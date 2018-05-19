#import "GameView.h"
#include "CocoaTouchWindow.h"
#include <ui/InputContext.h>
#include <ui/PointerInput.h>
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
    
    if (AppWindowInputSink *sink = _appWindow->GetInputSink())
    {
        auto touchIndex = GetFreeTouchIndex(_touches);
        auto pointerID = GetPointerID(touchIndex);
        vec2d pxPointerPos{
            static_cast<float>(location.x * self.contentScaleFactor),
            static_cast<float>(location.y * self.contentScaleFactor)};
        sink->OnPointer(UI::PointerType::Touch,
						UI::Msg::TAP,
						pxPointerPos,
						vec2d{}, // pxPointerOffset
						0, // buttons
						pointerID);
    }
}

- (void)handlePan:(UIPanGestureRecognizer *)recognizer
{
    CGPoint location = [recognizer locationInView: recognizer.view];
    CGPoint translation = [recognizer translationInView: recognizer.view];
    [recognizer setTranslation:CGPointMake(0, 0) inView: recognizer.view];
    
    if (AppWindowInputSink *sink = _appWindow->GetInputSink())
    {
        auto touchIndex = GetFreeTouchIndex(_touches);
        auto pointerID = GetPointerID(touchIndex);
        vec2d pxPointerPos{
            static_cast<float>(location.x * self.contentScaleFactor),
            static_cast<float>(location.y * self.contentScaleFactor)};
		vec2d pxPointerOffset = {
			static_cast<float>(translation.x * self.contentScaleFactor),
			static_cast<float>(translation.y * self.contentScaleFactor)};
        sink->OnPointer(UI::PointerType::Touch,
						UI::Msg::ScrollPrecise,
						pxPointerPos,
						pxPointerOffset,
						0, // buttons
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
     
        if (AppWindowInputSink *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->OnPointer(UI::PointerType::Touch,
							UI::Msg::PointerDown,
							pxPointerPos,
							vec2d{}, // pxPointerOffset
							1, // buttons
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
        if (AppWindowInputSink *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->OnPointer(UI::PointerType::Touch,
							UI::Msg::PointerMove,
							pxPointerPos,
							vec2d{}, // pxPointerOffset
							0, // buttons
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
        if (AppWindowInputSink *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->OnPointer(UI::PointerType::Touch,
							UI::Msg::PointerUp,
							pxPointerPos,
							vec2d{}, // pxPointerOffset
							1, // buttons
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
        if (AppWindowInputSink *sink = _appWindow->GetInputSink())
        {
            CGPoint location = [touch locationInView:self];
            vec2d pxPointerPos{
                static_cast<float>(location.x * self.contentScaleFactor),
                static_cast<float>(location.y * self.contentScaleFactor)};
            sink->OnPointer(UI::PointerType::Touch,
							UI::Msg::PointerCancel,
							pxPointerPos,
							vec2d{}, // pxPointerOffset
							0, // buttons
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

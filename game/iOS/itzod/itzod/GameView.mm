#import "GameView.h"
#include "CocoaTouchWindow.h"
#include <ui/GuiManager.h>

@interface GameView ()
{
    std::shared_ptr<CocoaTouchWindow> _appWindow;
}

@property (nonatomic) UITapGestureRecognizer* singleFingerTap;

@end


@implementation GameView

- (void)handleSingleTap:(UITapGestureRecognizer *)recognizer
{
    CGPoint location = [recognizer locationInView: recognizer.view];
    
    if (UI::LayoutManager *sink = _appWindow->GetInputSink())
    {
        sink->ProcessPointer(location.x * self.contentScaleFactor,
                             location.y * self.contentScaleFactor, 0, UI::Msg::TAP);
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
    
    self.singleFingerTap = [[UITapGestureRecognizer alloc]
                            initWithTarget:self
                            action:@selector(handleSingleTap:)];
    [self addGestureRecognizer:self.singleFingerTap];
    
    self.multipleTouchEnabled = YES;
    return self;
}

- (void)touchesBegan:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    [super touchesBegan: touches withEvent: event];
}

- (void)touchesMoved:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    [super touchesMoved:touches withEvent:event];
}

- (void)touchesEnded:(NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    [super touchesEnded:touches withEvent:event];
}

- (void)touchesCancelled:(nullable NSSet<UITouch *> *)touches withEvent:(nullable UIEvent *)event
{
    [super touchesCancelled:touches withEvent:event];
}

- (void)touchesEstimatedPropertiesUpdated:(NSSet * _Nonnull)touches
{
    [super touchesEstimatedPropertiesUpdated: touches];
}

@end

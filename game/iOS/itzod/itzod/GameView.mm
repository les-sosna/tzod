#import "GameView.h"
#include "CocoaTouchWindow.h"

@interface GameView ()
{
    std::shared_ptr<CocoaTouchWindow> _appWindow;
}

@end


@implementation GameView

- (AppWindow&)appWindow
{
    return *_appWindow;
}


- (instancetype) initWithCoder:(NSCoder *)aDecoder
{
    self = [super initWithCoder:aDecoder];
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

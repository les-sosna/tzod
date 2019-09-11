#import <GLKit/GLKit.h>
class CocoaTouchWindow;

@interface GameView : GLKView

@property (readonly, nonatomic) CocoaTouchWindow& appWindow;

@end

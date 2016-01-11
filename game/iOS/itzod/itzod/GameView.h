#import <GLKit/GLKit.h>
struct AppWindow;

@interface GameView : GLKView

@property (readonly, nonatomic) AppWindow& appWindow;

@end

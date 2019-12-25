#import <GLKit/GLKit.h>
class CocoaTouchWindow;
namespace Plat
{
    struct AppWindowInputSink;
}

@interface GameView : GLKView

@property (readonly, nonatomic) CocoaTouchWindow& appWindow;
@property (nonatomic) Plat::AppWindowInputSink* inputSink;

@end

#import <UIKit/UIKit.h>

class TzodApp;
namespace FS
{
    class FileSystem;
}
namespace UI
{
    class ConsoleBuffer;
}

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (readonly, nonatomic) FS::FileSystem &fs;
@property (readonly, nonatomic) UI::ConsoleBuffer &logger;
@property (readonly, nonatomic) TzodApp &app;

@end

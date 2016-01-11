#import <UIKit/UIKit.h>

class TzodApp;
namespace FS {
    class FileSystem;
}
namespace UI {
    class ConsoleBuffer;
}

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (readonly, nonatomic) FS::FileSystem &fs;
@property (readonly, nonatomic) UI::ConsoleBuffer &logger;
@property (readonly, nonatomic) TzodApp &app;

// The app delegate must implement the window property if it wants to use a main storyboard file.
@property (strong, nonatomic) UIWindow *window;

@end

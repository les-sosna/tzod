#import <UIKit/UIKit.h>

class AppController;
class AppState;
namespace FS
{
    class FileSystem;
}

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (readonly, nonatomic) FS::FileSystem &fs;
@property (readonly, nonatomic) AppController &appController;
@property (readonly, nonatomic) AppState &appState;

@end

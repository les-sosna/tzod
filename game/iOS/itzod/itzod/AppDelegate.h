#import <UIKit/UIKit.h>

namespace FS
{
    class FileSystem;
}

@interface AppDelegate : UIResponder <UIApplicationDelegate>

@property (strong, nonatomic) UIWindow *window;
@property (readonly, nonatomic) FS::FileSystem *fs;

@end

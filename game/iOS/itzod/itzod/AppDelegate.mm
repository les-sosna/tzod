#import "AppDelegate.h"
#include <app/tzod.h>
#include <fs/FileSystem.h>
#include <ui/ConsoleBuffer.h>
#include <memory>

@interface AppDelegate ()
{
    std::shared_ptr<FS::FileSystem> _fs;
    std::unique_ptr<TzodApp> _app;
    std::unique_ptr<UI::ConsoleBuffer> _logger;
}

@end


@implementation AppDelegate

- (FS::FileSystem&)fs
{
    return *_fs;
}

- (TzodApp&)app
{
    return *_app;
}

- (UI::ConsoleBuffer&)logger
{
    return *_logger;
}

- (BOOL)application:(UIApplication *)application didFinishLaunchingWithOptions:(NSDictionary *)launchOptions
{
    // Override point for customization after application launch.
    NSString *resourcePath = [[NSBundle mainBundle] resourcePath];
    NSString *dataPath = [resourcePath stringByAppendingPathComponent:@"data"];
    _fs = FS::CreateOSFileSystem([dataPath UTF8String]);
    _logger.reset(new UI::ConsoleBuffer(100, 500));
    _app.reset(new TzodApp(*_fs, *_logger));
    return YES;
}

- (void)applicationWillResignActive:(UIApplication *)application
{
    // Sent when the application is about to move from active to inactive state. This can occur for certain types of temporary interruptions (such as an incoming phone call or SMS message) or when the user quits the application and it begins the transition to the background state.
    // Use this method to pause ongoing tasks, disable timers, and throttle down OpenGL ES frame rates. Games should use this method to pause the game.
}

- (void)applicationDidEnterBackground:(UIApplication *)application
{
    // Use this method to release shared resources, save user data, invalidate timers, and store enough application state information to restore your application to its current state in case it is terminated later.
    // If your application supports background execution, this method is called instead of applicationWillTerminate: when the user quits.
}

- (void)applicationWillEnterForeground:(UIApplication *)application
{
    // Called as part of the transition from the background to the inactive state; here you can undo many of the changes made on entering the background.
}

- (void)applicationDidBecomeActive:(UIApplication *)application
{
    // Restart any tasks that were paused (or not yet started) while the application was inactive. If the application was previously in the background, optionally refresh the user interface.
}

- (void)applicationWillTerminate:(UIApplication *)application
{
    // Called when the application is about to terminate. Save data if appropriate. See also applicationDidEnterBackground:.
}

@end // @implementation AppDelegate

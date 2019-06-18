#include <plat/Folders.h>
#import <Cocoa/Cocoa.h>

std::string Plat::GetAppDataFolder()
{
	NSArray *paths = NSSearchPathForDirectoriesInDomains(NSApplicationSupportDirectory, NSUserDomainMask, YES /*expandTilde*/);
	NSString *applicationSupportDirectory = paths.firstObject;
	return applicationSupportDirectory.UTF8String;
}

std::string Plat::GetBundleResourcesFolder()
{
	return NSBundle.mainBundle.resourcePath.UTF8String;
}

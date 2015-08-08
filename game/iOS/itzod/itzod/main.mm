#import "AppDelegate.h"
#import <UIKit/UIKit.h>
#import <string>

// recursively print exception whats:
static void print_what(const std::exception &e, std::string prefix = std::string())
{
    NSLog(@"%s%s", prefix.c_str(), e.what());
    try {
        std::rethrow_if_nested(e);
    } catch (const std::exception& nested) {
        print_what(nested, prefix + "> ");
    }
}


int main(int argc, char * argv[])
{
    @autoreleasepool
    {
        try {
            return UIApplicationMain(argc, argv, nil, NSStringFromClass([AppDelegate class]));
        } catch (const std::exception &e) {
            print_what(e);
            throw;
        }
    }
}

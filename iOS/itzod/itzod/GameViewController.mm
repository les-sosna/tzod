#import "GameViewController.h"
#import "AppDelegate.h"
#import "GameView.h"
#import <OpenGLES/ES2/glext.h>
#include "CocoaTouchWindow.h"
#include <app/tzod.h>
#include <app/View.h>
#include <memory>

@interface GameViewController ()
{
    std::unique_ptr<TzodView> _tzodView;
}

- (void)tearDownGL;

@end


@implementation GameViewController


- (void)viewDidLoad
{
    [super viewDidLoad];

    self.preferredFramesPerSecond = 60;
    EAGLContext *context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!context)
    {
        NSLog(@"Failed to create ES context");
    }
    else
    {
        auto view = (GameView *)self.view;
        view.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
        view.context = context; // has to be set first to access view.appWindow property

        AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
        _tzodView.reset(new TzodView(appDelegate.fs, appDelegate.logger, appDelegate.app, view.appWindow));

        view.inputSink = &_tzodView->GetAppWindowInputSink();
    }
}

- (void)dealloc
{    
    [self tearDownGL];
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil))
    {
        self.view = nil;
        [self tearDownGL];
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)tearDownGL
{
    // TODO: consider deleting view's IRender instance
    _tzodView.reset();
    GLKView *view = (GLKView *)self.view;
    if ([EAGLContext currentContext] == view.context)
    {
        [EAGLContext setCurrentContext:nil];
    }
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    if (_tzodView)
    {
        AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
        _tzodView->Step(appDelegate.app, self.timeSinceLastUpdate);
    }
}

// GLKViewDelegate
- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (_tzodView)
    {
        auto &appWindow = ((GameView *)view).appWindow;
        _tzodView->GetAppWindowInputSink().OnRefresh(appWindow);
    }
}

@end // @implementation GameViewController

#import "GameViewController.h"
#import "AppDelegate.h"
#import "GameView.h"
#import "CocoaTouchWindow.h"
#import <OpenGLES/ES2/glext.h>
#include <app/tzod.h>
#include <app/View.h>
#include <video/RenderOpenGL.h>
#include <memory>

@interface GameViewController ()
{
    std::unique_ptr<CocoaTouchWindow> _appWindow;
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
        GameView *view = (GameView *)self.view;
        view.context = context;
        view.drawableDepthFormat = GLKViewDrawableDepthFormatNone;
        
        [EAGLContext setCurrentContext:context];

        AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
        _appWindow.reset(new CocoaTouchWindow(view));
        _tzodView.reset(new TzodView(appDelegate.fs, appDelegate.logger, appDelegate.app, *_appWindow));
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
    _tzodView.reset();
    _appWindow.reset();
    GLKView *view = (GLKView *)self.view;
    if ([EAGLContext currentContext] == view.context)
    {
        [EAGLContext setCurrentContext:nil];
    }
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    appDelegate.app.Step(self.timeSinceLastUpdate);
    if (_tzodView)
        _tzodView->Step(self.timeSinceLastUpdate);
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    if (_appWindow)
        _appWindow->SetPixelSize(static_cast<unsigned int>(view.drawableWidth),
                                 static_cast<unsigned int>(view.drawableHeight));
    
    if (_tzodView && _appWindow)
        _tzodView->Render(*_appWindow);
}

@end // @implementation GameViewController

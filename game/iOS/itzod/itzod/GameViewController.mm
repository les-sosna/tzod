#import "GameViewController.h"
#import "AppDelegate.h"
#import <OpenGLES/ES2/glext.h>
#include <app/AppCfg.h>
#include <app/AppState.h>
#include <app/GameContext.h>
#include <app/GameView.h>
#include <app/GameViewHarness.h>
#include <fs/FileSystem.h>
#include <render/RenderScheme.h>
#include <render/WorldView.h>
#include <video/DrawingContext.h>
#include <video/RenderOpenGL.h>
#include <video/TextureManager.h>
#include <memory>

@interface GameViewController ()
{
    std::unique_ptr<IRender> _render;
    std::unique_ptr<TextureManager> _textureManager;
    std::unique_ptr<RenderScheme> _renderScheme;
    std::unique_ptr<WorldView> _worldView;
    std::unique_ptr<GameView> _gameView;
}
@property (strong, nonatomic) EAGLContext *context;
@property (strong, nonatomic) GLKBaseEffect *effect;

- (void)setupGL;
- (void)tearDownGL;

@end

@implementation GameViewController

- (void)viewDidLoad
{
    [super viewDidLoad];
    
    self.context = [[EAGLContext alloc] initWithAPI:kEAGLRenderingAPIOpenGLES2];

    if (!self.context) {
        NSLog(@"Failed to create ES context");
    }
    
    GLKView *view = (GLKView *)self.view;
    view.context = self.context;
    view.drawableDepthFormat = GLKViewDrawableDepthFormat24;
    
    [self setupGL];
}

- (void)dealloc
{    
    [self tearDownGL];
    
    if ([EAGLContext currentContext] == self.context)
    {
        [EAGLContext setCurrentContext:nil];
    }
}

- (void)didReceiveMemoryWarning
{
    [super didReceiveMemoryWarning];

    if ([self isViewLoaded] && ([[self view] window] == nil))
    {
        self.view = nil;
        
        [self tearDownGL];
        
        if ([EAGLContext currentContext] == self.context)
        {
            [EAGLContext setCurrentContext:nil];
        }
        self.context = nil;
    }

    // Dispose of any resources that can be recreated.
}

- (BOOL)prefersStatusBarHidden
{
    return YES;
}

- (void)setupGL
{
    [EAGLContext setCurrentContext:self.context];
    
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    
    _render = RenderCreateOpenGL();
    _textureManager.reset(new TextureManager(*_render));
    FS::FileSystem &fs = appDelegate.fs;
    if (_textureManager->LoadPackage(FILE_TEXTURES, fs.Open(FILE_TEXTURES)->QueryMap(), fs) <= 0)
        NSLog(@"WARNING: no textures loaded");
    if (_textureManager->LoadDirectory(DIR_SKINS, "skin/", fs) <= 0)
        NSLog(@"WARNING: no skins found");

    _renderScheme.reset(new RenderScheme(*_textureManager));
    _worldView.reset(new WorldView(*_textureManager, *_renderScheme));
    _gameView.reset(new GameView(appDelegate.appState));
}

- (void)tearDownGL
{
    [EAGLContext setCurrentContext:self.context];

    _worldView.reset();
    _renderScheme.reset();
    _textureManager.reset();
    _render.reset();
}

#pragma mark - GLKView and GLKViewController delegate methods

- (void)update
{
    AppDelegate *appDelegate = (AppDelegate *)[[UIApplication sharedApplication] delegate];
    if (GameContextBase *gc = [appDelegate appState].GetGameContext())
    {
        gc->Step(self.timeSinceLastUpdate);
    }
    if (GameViewHarness *gvh = _gameView->GetHarness())
    {
        GLKView *view = (GLKView *)self.view;
        gvh->Step(self.timeSinceLastUpdate, view.drawableWidth, view.drawableHeight);
    }
}

- (void)glkView:(GLKView *)view drawInRect:(CGRect)rect
{
    DrawingContext dc(*_textureManager, view.drawableWidth, view.drawableHeight);
    
    if (GameViewHarness *gvh = _gameView->GetHarness())
    {
        _render->Begin();
        gvh->RenderGame(dc, *_worldView, view.drawableWidth, view.drawableHeight, vec2d(0,0), 1.0f);
        _render->End();
    }
}

@end // @implementation GameViewController

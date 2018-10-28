#include <jni.h>
#include <android/log.h>

#include <GLES2/gl2.h>
#include <app/tzod.h>
#include <app/View.h>
#include <fsjni/FileSystemJni.h>
#include <plat/ConsoleBuffer.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#define  LOG_TAG    "libtzodjni"
#define  LOGI(...)  __android_log_print(ANDROID_LOG_INFO,LOG_TAG,__VA_ARGS__)

static void printGLString(const char *name, GLenum s) {
    const char *v = (const char *) glGetString(s);
    LOGI("GL %s = %s\n", name, v);
}

#include <platjni/JniAppWindow.h>
#include <platjni/JniConsoleLog.h>
#include <android/asset_manager_jni.h>

struct State
{
    Plat::ConsoleBuffer logger;
    std::shared_ptr<FS::FileSystem> fs;
    TzodApp app;
    JniAppWindow appWindow;
    TzodView view;

    State(AAssetManager *assetManager)
        : logger(80, 100)
        , fs(std::make_shared<FS::FileSystemJni>(assetManager, "data"))
        , app(*fs, logger)
        , view(*fs, logger, app, appWindow)
    {
        logger.SetLog(new JniConsoleLog());
    }
};

std::unique_ptr<State> g_state;

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_init(JNIEnv *env, jobject obj, jobject assetManager)
{
    g_state = std::make_unique<State>(AAssetManager_fromJava(env, assetManager));

    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);
}

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_resize(JNIEnv *env, jobject obj, jint width, jint height)
{
    g_state->appWindow.SetPixelSize(vec2d{static_cast<float>(width), static_cast<float>(height)});
}

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_step(JNIEnv *env, jobject obj)
{
    g_state->view.Step(0.16);
}

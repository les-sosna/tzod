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

static void checkGlError(const char* op) {
    for (GLint error = glGetError(); error; error
            = glGetError()) {
        LOGI("after %s() glError (0x%x)\n", op, error);
    }
}

bool setupGraphics(int w, int h) {
    printGLString("Version", GL_VERSION);
    printGLString("Vendor", GL_VENDOR);
    printGLString("Renderer", GL_RENDERER);
    printGLString("Extensions", GL_EXTENSIONS);

    LOGI("setupGraphics(%d, %d)", w, h);

    glViewport(0, 0, w, h);
    checkGlError("glViewport");
    return true;
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

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_init(JNIEnv * env, jobject obj, jobject assetManager, jint width, jint height)
{
    g_state = std::make_unique<State>(AAssetManager_fromJava(env, assetManager));
    setupGraphics(width, height);
}

extern "C" JNIEXPORT void JNICALL Java_com_neaoo_tzod_TZODJNILib_step(JNIEnv * env, jobject obj)
{
    g_state->view.Step(0.16);
}

package com.neaoo.tzod;

import android.content.Context;
import android.content.res.AssetManager;
import android.graphics.PixelFormat;
import android.opengl.GLSurfaceView;
import android.view.MotionEvent;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.opengles.GL10;

/**
 *
 * - The class must use a custom context factory to enable 2.0 rendering.
 *   See ContextFactory class definition below.
 *
 * - The class must use a custom EGLConfigChooser to be able to select
 *   an EGLConfig that supports 2.0. This is done by providing a config
 *   specification to eglChooseConfig() that has the attribute
 *   EGL10.ELG_RENDERABLE_TYPE containing the EGL_OPENGL_ES2_BIT flag
 *   set. See ConfigChooser class definition below.
 *
 * - The class must select the surface's format, then choose an EGLConfig
 *   that matches it exactly (with regards to red/green/blue/alpha channels
 *   bit depths). Failure to do so would result in an EGL_BAD_MATCH error.
 */
class TZODJNIView extends GLSurfaceView {
    public TZODJNIView(Context context) {
        super(context);

        /* By default, GLSurfaceView() creates a RGB_565 opaque surface.
         * If we want a translucent one, we should change the surface's
         * format here, using PixelFormat.TRANSLUCENT for GL Surfaces
         * is interpreted as any 32-bit surface with alpha by SurfaceFlinger.
         */
        this.getHolder().setFormat(PixelFormat.TRANSLUCENT);

        /* Setup the context factory for 2.0 rendering.
         * See ContextFactory class definition below
         */
        setEGLContextFactory(new ContextFactoryES20());

        setEGLConfigChooser(new ConfigChooser());

        /* Set the renderer responsible for frame rendering */
        setRenderer(new Renderer(context.getAssets()));
    }

    @Override
    public boolean onTouchEvent(MotionEvent e) {
        int actionMasked = e.getActionMasked();
        switch (actionMasked) {
            case MotionEvent.ACTION_DOWN:
            case MotionEvent.ACTION_POINTER_DOWN:
            case MotionEvent.ACTION_UP:
            case MotionEvent.ACTION_POINTER_UP:
            case MotionEvent.ACTION_MOVE:
                int pointerIndex = e.getActionIndex();
                TZODJNILib.pointer(actionMasked, e.getPointerId(pointerIndex), e.getX(pointerIndex), e.getY(pointerIndex));
        }

        return true;
    }

    private static class ContextFactoryES20 implements GLSurfaceView.EGLContextFactory {
        private static int EGL_CONTEXT_CLIENT_VERSION = 0x3098;
        public EGLContext createContext(EGL10 egl, EGLDisplay display, EGLConfig eglConfig) {
            int[] attributes = {
                EGL_CONTEXT_CLIENT_VERSION, 2,
                EGL10.EGL_NONE
            };
            return egl.eglCreateContext(display, eglConfig, EGL10.EGL_NO_CONTEXT, attributes);
        }

        public void destroyContext(EGL10 egl, EGLDisplay display, EGLContext context) {
            egl.eglDestroyContext(display, context);
        }
    }

    private static class ConfigChooser implements GLSurfaceView.EGLConfigChooser {

        private static final int RED_SIZE = 8;
        private static final int GREEN_SIZE = 8;
        private static final int BLUE_SIZE = 8;
        private static final int ALPHA_SIZE = 8;
        private static final int EGL_OPENGL_ES2_BIT = 4;
        private static int[] s_configAttribs2 =
        {
            EGL10.EGL_RED_SIZE, RED_SIZE,
            EGL10.EGL_GREEN_SIZE, GREEN_SIZE,
            EGL10.EGL_BLUE_SIZE, BLUE_SIZE,
            EGL10.EGL_ALPHA_SIZE, ALPHA_SIZE,
            EGL10.EGL_RENDERABLE_TYPE, EGL_OPENGL_ES2_BIT,
            EGL10.EGL_NONE
        };

        @Override
        public EGLConfig chooseConfig(EGL10 egl, EGLDisplay display) {

            int[] num_config = new int[1];
            egl.eglChooseConfig(display, s_configAttribs2, null, 0, num_config);

            int numConfigs = num_config[0];
            if (numConfigs <= 0) {
                throw new IllegalArgumentException("No configs match configSpec");
            }

            EGLConfig[] configs = new EGLConfig[numConfigs];
            egl.eglChooseConfig(display, s_configAttribs2, configs, numConfigs, num_config);

            return chooseConfig(egl, display, configs);
        }

        private EGLConfig chooseConfig(EGL10 egl, EGLDisplay display, EGLConfig[] configs) {
            for(EGLConfig config : configs) {
                int r = findConfigAttrib(egl, display, config, EGL10.EGL_RED_SIZE, 0);
                int g = findConfigAttrib(egl, display, config, EGL10.EGL_GREEN_SIZE, 0);
                int b = findConfigAttrib(egl, display, config, EGL10.EGL_BLUE_SIZE, 0);
                int a = findConfigAttrib(egl, display, config, EGL10.EGL_ALPHA_SIZE, 0);

                if (r == RED_SIZE && g == GREEN_SIZE && b == BLUE_SIZE && a == ALPHA_SIZE)
                    return config;
            }
            return null;
        }

        private int findConfigAttrib(EGL10 egl, EGLDisplay display,
                EGLConfig config, int attribute, int defaultValue) {

            if (egl.eglGetConfigAttrib(display, config, attribute, mValue)) {
                return mValue[0];
            }
            return defaultValue;
        }
        private int[] mValue = new int[1];
    }

    private static class Renderer implements GLSurfaceView.Renderer {
        private AssetManager _assetManager;

        private Renderer(AssetManager assetManager) {
            _assetManager = assetManager;
        }

        @Override
        public void onDrawFrame(GL10 gl) { TZODJNILib.step(); }

        @Override
        public void onSurfaceChanged(GL10 gl, int width, int height) {
            TZODJNILib.resize(width, height);
        }

        @Override
        public void onSurfaceCreated(GL10 gl, EGLConfig config) {
            TZODJNILib.init(_assetManager);
        }
    }
}

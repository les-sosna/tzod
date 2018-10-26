package com.neaoo.tzod;

// Wrapper for native library

import android.content.res.AssetManager;

public class TZODJNILib {

    static {
        System.loadLibrary("tzodjni");
    }

    /**
     * @param width the current view width
     * @param height the current view height
     */
    public static native void init(AssetManager assetManager, int width, int height);
    public static native void step();
}

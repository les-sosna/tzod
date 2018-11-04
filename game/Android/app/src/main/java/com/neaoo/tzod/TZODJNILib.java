package com.neaoo.tzod;

import android.content.res.AssetManager;

public class TZODJNILib {

    static {
        System.loadLibrary("tzodjni");
    }

    public static native void init(AssetManager assetManager);
    public static native void resize(int width, int height);
    public static native void step();

    public static native void tap(float x, float y);
}

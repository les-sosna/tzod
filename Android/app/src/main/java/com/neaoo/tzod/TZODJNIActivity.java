package com.neaoo.tzod;

import android.app.Activity;
import android.os.Bundle;

public class TZODJNIActivity extends Activity {

    TZODJNIView mView;

    @Override protected void onCreate(Bundle icicle) {
        super.onCreate(icicle);
        mView = new TZODJNIView(getApplication());
        setContentView(mView);
    }

    @Override protected void onPause() {
        super.onPause();
        mView.onPause();
    }

    @Override protected void onResume() {
        super.onResume();
        mView.onResume();
    }
}

package com.beatsbybony.cooledit;

import android.os.Bundle;
import com.getcapacitor.BridgeActivity;

public class MainActivity extends BridgeActivity {
    @Override
    public void onCreate(Bundle savedInstanceState) {
        registerPlugin(MixCapturePlugin.class);
        super.onCreate(savedInstanceState);
    }
}

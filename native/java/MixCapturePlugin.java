package com.beatsbybony.cooledit;

import android.Manifest;
import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjectionManager;
import android.os.Build;

import androidx.activity.result.ActivityResult;
import androidx.core.content.ContextCompat;

import com.getcapacitor.JSObject;
import com.getcapacitor.PermissionState;
import com.getcapacitor.Plugin;
import com.getcapacitor.PluginCall;
import com.getcapacitor.PluginMethod;
import com.getcapacitor.annotation.ActivityCallback;
import com.getcapacitor.annotation.CapacitorPlugin;
import com.getcapacitor.annotation.Permission;
import com.getcapacitor.annotation.PermissionCallback;

/** Records the audio that other apps play (Android 10+), using playback capture. */
@CapacitorPlugin(
    name = "MixCapture",
    permissions = { @Permission(strings = { Manifest.permission.RECORD_AUDIO }, alias = "mic") }
)
public class MixCapturePlugin extends Plugin {

    @PluginMethod
    public void start(PluginCall call) {
        if (Build.VERSION.SDK_INT < 29) {
            call.reject("Recording other apps needs Android 10 or newer.");
            return;
        }
        if (getPermissionState("mic") != PermissionState.GRANTED) {
            requestPermissionForAlias("mic", call, "micPermResult");
            return;
        }
        launchProjection(call);
    }

    @PermissionCallback
    private void micPermResult(PluginCall call) {
        if (getPermissionState("mic") == PermissionState.GRANTED) {
            launchProjection(call);
        } else {
            call.reject("Audio recording permission is needed to capture sound.");
        }
    }

    private void launchProjection(PluginCall call) {
        MediaProjectionManager m =
            (MediaProjectionManager) getContext().getSystemService(Context.MEDIA_PROJECTION_SERVICE);
        startActivityForResult(call, m.createScreenCaptureIntent(), "projectionResult");
    }

    @ActivityCallback
    private void projectionResult(PluginCall call, ActivityResult result) {
        if (call == null) return;
        if (result.getResultCode() != Activity.RESULT_OK || result.getData() == null) {
            call.reject("Capture was not allowed.");
            return;
        }
        MixCaptureService.state = 0;
        MixCaptureService.error = null;
        Intent i = new Intent(getContext(), MixCaptureService.class);
        i.putExtra("rc", result.getResultCode());
        i.putExtra("data", result.getData());
        ContextCompat.startForegroundService(getContext(), i);
        new Thread(() -> {
            long t0 = System.currentTimeMillis();
            while (MixCaptureService.state == 0 && System.currentTimeMillis() - t0 < 5000) {
                try { Thread.sleep(40); } catch (InterruptedException ignored) { }
            }
            if (MixCaptureService.state == 1) {
                call.resolve();
            } else {
                String e = MixCaptureService.error;
                call.reject(e != null ? e : "Could not start capture.");
            }
        }).start();
    }

    @PluginMethod
    public void stop(PluginCall call) {
        new Thread(() -> {
            String path = MixCaptureService.stopCapture();
            if (path == null) {
                call.reject("No recording in progress.");
                return;
            }
            JSObject o = new JSObject();
            o.put("uri", "file://" + path);
            o.put("sampleRate", MixCaptureService.SAMPLE_RATE);
            o.put("channels", 2);
            call.resolve(o);
        }).start();
    }
}

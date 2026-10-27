package com.beatsbybony.cooledit;

import android.app.Notification;
import android.app.NotificationChannel;
import android.app.NotificationManager;
import android.app.Service;
import android.content.Intent;
import android.content.pm.ServiceInfo;
import android.media.AudioAttributes;
import android.media.AudioFormat;
import android.media.AudioPlaybackCaptureConfiguration;
import android.media.AudioRecord;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import android.os.Build;
import android.os.Handler;
import android.os.IBinder;
import android.os.Looper;

import java.io.File;
import java.io.FileOutputStream;

/** Foreground service that holds the screen-capture token and writes captured playback audio to a PCM file. */
public class MixCaptureService extends Service {
    static final int SAMPLE_RATE = 44100;
    static volatile int state = 0; // 0 starting, 1 recording, -1 failed
    static volatile String error = null;
    static volatile String outPath = null;
    private static volatile MixCaptureService inst = null;

    private MediaProjection projection;
    private AudioRecord rec;
    private Thread worker;
    private volatile boolean running = false;
    private FileOutputStream out;

    @Override
    public IBinder onBind(Intent intent) { return null; }

    @Override
    public int onStartCommand(Intent intent, int flags, int startId) {
        inst = this;
        try {
            NotificationManager nm = (NotificationManager) getSystemService(NOTIFICATION_SERVICE);
            if (Build.VERSION.SDK_INT >= 26) {
                nm.createNotificationChannel(new NotificationChannel("mixcap", "Recording", NotificationManager.IMPORTANCE_LOW));
            }
            Notification n = new Notification.Builder(this, "mixcap")
                .setContentTitle("Cool Edit Mobile")
                .setContentText("Recording audio from other apps")
                .setSmallIcon(android.R.drawable.ic_btn_speak_now)
                .setOngoing(true)
                .build();
            if (Build.VERSION.SDK_INT >= 29) {
                startForeground(4711, n, ServiceInfo.FOREGROUND_SERVICE_TYPE_MEDIA_PROJECTION);
            } else {
                startForeground(4711, n);
            }

            int rc = intent.getIntExtra("rc", 0);
            Intent data = intent.getParcelableExtra("data");
            MediaProjectionManager mgr = (MediaProjectionManager) getSystemService(MEDIA_PROJECTION_SERVICE);
            projection = mgr.getMediaProjection(rc, data);
            projection.registerCallback(new MediaProjection.Callback() {
                @Override public void onStop() { running = false; }
            }, new Handler(Looper.getMainLooper()));

            AudioPlaybackCaptureConfiguration cfg = new AudioPlaybackCaptureConfiguration.Builder(projection)
                .addMatchingUsage(AudioAttributes.USAGE_MEDIA)
                .addMatchingUsage(AudioAttributes.USAGE_GAME)
                .addMatchingUsage(AudioAttributes.USAGE_UNKNOWN)
                .build();
            AudioFormat fmt = new AudioFormat.Builder()
                .setEncoding(AudioFormat.ENCODING_PCM_16BIT)
                .setSampleRate(SAMPLE_RATE)
                .setChannelMask(AudioFormat.CHANNEL_IN_STEREO)
                .build();
            int min = AudioRecord.getMinBufferSize(SAMPLE_RATE, AudioFormat.CHANNEL_IN_STEREO, AudioFormat.ENCODING_PCM_16BIT);
            rec = new AudioRecord.Builder()
                .setAudioFormat(fmt)
                .setBufferSizeInBytes(Math.max(min, 65536))
                .setAudioPlaybackCaptureConfig(cfg)
                .build();
            if (rec.getState() != AudioRecord.STATE_INITIALIZED) {
                throw new IllegalStateException("Audio capture could not be initialised on this device.");
            }

            File f = new File(getCacheDir(), "mix_" + System.currentTimeMillis() + ".pcm");
            out = new FileOutputStream(f);
            outPath = f.getAbsolutePath();
            rec.startRecording();
            running = true;
            worker = new Thread(() -> {
                byte[] buf = new byte[16384];
                try {
                    while (running) {
                        int r = rec.read(buf, 0, buf.length);
                        if (r > 0) out.write(buf, 0, r);
                        else if (r < 0) break;
                    }
                } catch (Exception ignored) { }
            }, "mixcap");
            worker.start();
            state = 1;
        } catch (Exception e) {
            error = e.getMessage() != null ? e.getMessage() : e.toString();
            state = -1;
            releaseAll();
            stopSelf();
        }
        return START_NOT_STICKY;
    }

    private void releaseAll() {
        running = false;
        try { if (worker != null) worker.join(2000); } catch (InterruptedException ignored) { }
        try { if (rec != null) { rec.stop(); } } catch (Exception ignored) { }
        try { if (rec != null) { rec.release(); } } catch (Exception ignored) { }
        try { if (out != null) { out.close(); } } catch (Exception ignored) { }
        try { if (projection != null) { projection.stop(); } } catch (Exception ignored) { }
        rec = null; out = null; projection = null; worker = null;
    }

    /** Stops the capture and returns the PCM file path (16-bit stereo, 44100 Hz), or null if nothing was recording. */
    static String stopCapture() {
        MixCaptureService s = inst;
        if (s == null || state != 1) return null;
        s.releaseAll();
        state = 0;
        try { s.stopForeground(true); } catch (Exception ignored) { }
        s.stopSelf();
        inst = null;
        return outPath;
    }

    @Override
    public void onDestroy() {
        releaseAll();
        super.onDestroy();
    }
}

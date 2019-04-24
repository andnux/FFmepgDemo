package top.andnux.ffmepgdemo;

import android.view.Surface;

public class FFmpeg {

    public static void loadLibrary() {
        System.loadLibrary("avutil-56");
        System.loadLibrary("swresample-3");
        System.loadLibrary("swscale-5");
        System.loadLibrary("avcodec-58");
        System.loadLibrary("avformat-58");
        System.loadLibrary("postproc-55");
        System.loadLibrary("avfilter-7");
        System.loadLibrary("avdevice-58");
        System.loadLibrary("ffmpeg");
    }

    static {
        loadLibrary();
    }

    public static native void sound(String input,String output);
    public static native void decode(String input, Surface surface);
}

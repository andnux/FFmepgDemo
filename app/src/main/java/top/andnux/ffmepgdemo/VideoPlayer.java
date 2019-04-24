package top.andnux.ffmepgdemo;

import android.content.Context;
import android.graphics.PixelFormat;
import android.os.Build;
import android.util.AttributeSet;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;

import androidx.annotation.RequiresApi;

public class VideoPlayer extends SurfaceView {

    private SurfaceHolder mHolder;
    private Surface mSurface;

    public VideoPlayer(Context context) {
        this(context, null);
    }

    public VideoPlayer(Context context, AttributeSet attrs) {
        this(context, attrs, 0);
    }

    public VideoPlayer(Context context, AttributeSet attrs, int defStyleAttr) {
        super(context, attrs, defStyleAttr);
        init(context, attrs);
    }

    @RequiresApi(api = Build.VERSION_CODES.LOLLIPOP)
    public VideoPlayer(Context context, AttributeSet attrs, int defStyleAttr, int defStyleRes) {
        super(context, attrs, defStyleAttr, defStyleRes);
        init(context, attrs);
    }


    private void init(Context context, AttributeSet attrs) {
        mHolder = getHolder();
        mHolder.setFormat(PixelFormat.RGBA_8888);
        mHolder.setKeepScreenOn(true);
        mSurface = mHolder.getSurface();
    }

    public void play(String path){
        FFmpeg.decode(path,mSurface);
    }
}

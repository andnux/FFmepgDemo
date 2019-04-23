package top.andnux.ffmepgdemo;

import android.app.IntentService;
import android.content.Intent;
import android.util.Log;

import androidx.annotation.Nullable;

public class InitService extends IntentService {

    public InitService() {
        super("InitService");
    }

    @Override
    protected void onHandleIntent(@Nullable Intent intent) {
        long start = System.currentTimeMillis();
        FFmpeg.loadLibrary();
        long eng = System.currentTimeMillis();
        Log.e("TAG", String.valueOf(eng - start));
        Log.e("TAG",Thread.currentThread().getName());
    }
}

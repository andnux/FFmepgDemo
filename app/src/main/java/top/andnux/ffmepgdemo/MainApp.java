package top.andnux.ffmepgdemo;

import android.app.Application;
import android.content.Intent;

public class MainApp extends Application {

    @Override
    public void onCreate() {
        super.onCreate();
        startService(new Intent(this,InitService.class));
    }
}

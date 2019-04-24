package top.andnux.ffmepgdemo;

import androidx.appcompat.app.AppCompatActivity;

import android.Manifest;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.util.Log;
import android.view.View;
import android.widget.TextView;

import java.io.File;
import java.io.FileInputStream;
import java.io.FileNotFoundException;

public class MainActivity extends AppCompatActivity {

    private VideoPlayer videoPlayer;

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);
        setContentView(R.layout.activity_main);
        videoPlayer = findViewById(R.id.videoPlayer);
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            requestPermissions(new String[]{
                    Manifest.permission.WRITE_EXTERNAL_STORAGE,
                    Manifest.permission.READ_EXTERNAL_STORAGE}, 0x00);
        }
    }

    public void onClick(View view) {
        File directory = Environment.getExternalStorageDirectory();
        final File in = new File(directory, "1.mp4");
        final File out = new File(directory, "1.yuv");
        videoPlayer.play(in.getAbsolutePath());
//        final AudioPlayer audioPlayer = new AudioPlayer(44100, 1);
//        audioPlayer.play();
//        new Thread() {
//            @Override
//            public void run() {
//                super.run();
//                try {
//                    FileInputStream fs = new FileInputStream(in);
//                    byte[] bytes = new byte[2 * 1024];
//                    int length = 0;
//                    while ((length = fs.read(bytes)) != -1) {
//                        audioPlayer.write(bytes, 0, length);
//                    }
//                    fs.close();
//                } catch (Exception e) {
//                    e.printStackTrace();
//                }
//                FFmpeg.decode(in.getAbsolutePath(), out.getAbsolutePath());
//            }
//        }.start();
    }
}

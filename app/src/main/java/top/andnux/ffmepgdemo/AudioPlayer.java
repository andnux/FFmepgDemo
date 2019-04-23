package top.andnux.ffmepgdemo;

import android.media.AudioFormat;
import android.media.AudioManager;
import android.media.AudioTrack;
import android.util.Log;

import androidx.annotation.Keep;

@Keep
public class AudioPlayer {

    private AudioTrack mTrack;

    @Keep
    public AudioPlayer(int sampleRateInHz,int nb_channels) {
        int audioFormat = AudioFormat.ENCODING_PCM_16BIT;
        int streamType = AudioManager.STREAM_MUSIC;
        int channelConfig;
        if(nb_channels == 1){
            channelConfig = AudioFormat.CHANNEL_OUT_MONO;
        }else if(nb_channels == 2){
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }else{
            channelConfig = AudioFormat.CHANNEL_OUT_STEREO;
        }
        int minBufferSize = AudioTrack.getMinBufferSize(sampleRateInHz,
                channelConfig, audioFormat);
        mTrack = new AudioTrack(streamType, sampleRateInHz,
                channelConfig, audioFormat, minBufferSize,  AudioTrack.MODE_STREAM);
        Log.e("TAG","sampleRateInHz = " + sampleRateInHz);
        Log.e("TAG","nb_channels = " + nb_channels);
    }

    @Keep
    public void play() {
        mTrack.play();
    }

    @Keep
    public int write(byte[] audioData, int sizeInBytes) {
        return mTrack.write(audioData, 0, sizeInBytes);
    }

    @Keep
    public int write(short[] audioData, int sizeInShorts) {
        return mTrack.write(audioData, 0, sizeInShorts);
    }

    @Keep
    public int write(byte[] audioData, int offsetInBytes, int sizeInBytes) {
        return mTrack.write(audioData, offsetInBytes, sizeInBytes);
    }

    @Keep
    public int write(short[] audioData, int offsetInShorts, int sizeInShorts) {
        return mTrack.write(audioData, offsetInShorts, sizeInShorts);
    }
}

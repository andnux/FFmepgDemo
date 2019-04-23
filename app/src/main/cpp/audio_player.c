//
// Created by 张春林 on 2019-04-22.
//
#include<jni.h>
#include <android/log.h>
#include <unistd.h>
#include <string.h>

#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libswresample/swresample.h"

#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,"andnux",FORMAT,__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"andnux",FORMAT,__VA_ARGS__)


JNIEXPORT void JNICALL Java_top_andnux_ffmepgdemo_FFmpeg_sound(JNIEnv *env,
                                                               jclass clazz, jstring input_jstr,
                                                               jstring output_jstr) {
    const char *input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
    const char *output_cstr = (*env)->GetStringUTFChars(env, output_jstr, NULL);

    LOGE("input_cstr = %s", input_cstr);
    //注册组件
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    //打开音频文件
    if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
        LOGE("%s", "无法打开音频文件");
        return;
    }
    //获取输入文件信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "无法获取输入文件信息");
        return;
    }
    //获取音频流索引位置
    int i = 0, audio_stream_idx = -1;
    for (; i < pFormatCtx->nb_streams; i++) {
        if (pFormatCtx->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_idx = i;
            break;
        }
    }

    //获取解码器
    AVCodecContext * codecCtx = pFormatCtx->streams[audio_stream_idx]->codec;
    AVCodec *codec = avcodec_find_decoder(codecCtx->codec_id);
    //打开解码器
    if (avcodec_open2(codecCtx, codec, NULL) < 0) {
        LOGE("%s", "无法打开解码器");
        return;
    }
    //压缩数据
    AVPacket *packet = (AVPacket *) av_malloc(sizeof(AVPacket));
    //解压缩数据
    AVFrame *frame = av_frame_alloc();
    //frame->16bit 44100 PCM 统一音频采样格式与采样率
    SwrContext *swrCtx = swr_alloc();

    //重采样设置参数-------------start
    //输入的采样格式
    enum AVSampleFormat in_sample_fmt = codecCtx->sample_fmt;
    //输出采样格式16bit PCM
    enum AVSampleFormat out_sample_fmt = AV_SAMPLE_FMT_S16;
    //输入采样率
    int in_sample_rate = codecCtx->sample_rate;
    //输出采样率
    int out_sample_rate = in_sample_rate;

    //获取输入的声道布局
    //根据声道个数获取默认的声道布局（2个声道，默认立体声stereo）
    //av_get_default_channel_layout(codecCtx->channels);
    uint64_t in_ch_layout = codecCtx->channel_layout;
    //输出的声道布局（立体声）
    uint64_t out_ch_layout = AV_CH_LAYOUT_STEREO;

    swr_alloc_set_opts(swrCtx,
                       out_ch_layout, out_sample_fmt, out_sample_rate,
                       in_ch_layout, in_sample_fmt, in_sample_rate,
                       0, NULL);
    swr_init(swrCtx);

    //输出的声道个数
    int out_channel_nb = av_get_channel_layout_nb_channels(out_ch_layout);

    //重采样设置参数-------------end

    //JNI begin------------------
    //AudioPlayer
    jclass audioPlayerClazz = (*env)->FindClass(env, "top/andnux/ffmepgdemo/AudioPlayer");

    //AudioTrack对象
    jmethodID audioPlayerInitMethod = (*env)->GetMethodID(env, audioPlayerClazz, "<init>", "(II)V");
    jobject audioPlayer = (*env)->NewObject(env, audioPlayerClazz, audioPlayerInitMethod,
                                            out_sample_rate, out_channel_nb);

    //调用AudioPlayer.play方法
    jmethodID playMethod = (*env)->GetMethodID(env, audioPlayerClazz, "play", "()V");
    (*env)->CallVoidMethod(env, audioPlayer, playMethod);

    //AudioPlayer.write
    jmethodID writeMethod = (*env)->GetMethodID(env, audioPlayerClazz, "write", "([BII)I");
    //JNI end------------------

    //16bit 44100 PCM 数据
    uint8_t *out_buffer = (uint8_t *) av_malloc(out_sample_rate * 2);

    int got_frame = 0, index = 0, ret;
    //不断读取压缩数据
    while (av_read_frame(pFormatCtx, packet) >= 0) {
        //解码音频类型的Packet
        if (packet->stream_index == audio_stream_idx) {
            //解码
            ret = avcodec_decode_audio4(codecCtx, frame, &got_frame, packet);

            if (ret < 0) {
                LOGD("%s", "解码完成");
            }
            //解码一帧成功
            if (got_frame > 0) {
                LOGD("解码：%d", index++);
                swr_convert(swrCtx, &out_buffer, out_sample_rate * 2,
                            (const uint8_t **) frame->data, frame->nb_samples);
                //获取sample的size
                int out_buffer_size = av_samples_get_buffer_size(NULL, out_channel_nb,
                                                                 frame->nb_samples, out_sample_fmt,
                                                                 1);

                //out_buffer缓冲区数据，转成byte数组
                jbyteArray audio_sample_array = (*env)->NewByteArray(env, out_buffer_size);
                jbyte *sample_bytep = (*env)->GetByteArrayElements(env, audio_sample_array, NULL);
                //out_buffer的数据复制到sampe_bytep
                memcpy(sample_bytep, out_buffer, out_buffer_size);
                //同步
                (*env)->ReleaseByteArrayElements(env, audio_sample_array, sample_bytep, 0);

                //AudioTrack.write PCM数据
                (*env)->CallIntMethod(env, audioPlayer, writeMethod,
                                      audio_sample_array, 0, out_buffer_size);
                //释放局部引用
                (*env)->DeleteLocalRef(env, audio_sample_array);

                usleep(1000 * 16);
            }
        }

        av_free_packet(packet);
    }

    av_frame_free(&frame);
    av_free(out_buffer);

    swr_free(&swrCtx);
    avcodec_close(codecCtx);
    avformat_close_input(&pFormatCtx);

    (*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);
    (*env)->ReleaseStringUTFChars(env, output_jstr, output_cstr);
}

#pragma clang diagnostic pop
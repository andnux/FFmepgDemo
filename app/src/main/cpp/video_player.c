#include<jni.h>
#include <android/log.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <android/native_window_jni.h>
#include <android/native_window.h>
#include <libswresample/swresample.h>
#include "libavutil/adler32.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"
#include "libyuv/libyuv.h"

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"

#define LOGD(FORMAT, ...) __android_log_print(ANDROID_LOG_DEBUG,"andnux",FORMAT,__VA_ARGS__)
#define LOGE(FORMAT, ...) __android_log_print(ANDROID_LOG_ERROR,"andnux",FORMAT,__VA_ARGS__)

static int video_decode(const char *filename) {
    AVCodec *codec = NULL;
    AVCodecContext *ctx = NULL;
    AVCodecParameters *origin_par = NULL;
    AVFrame *fr = NULL;
    uint8_t *byte_buffer = NULL;
    AVPacket pkt;
    AVFormatContext *fmt_ctx = NULL;
    int number_of_written_bytes;
    int video_stream;
    int got_frame = 0;
    int byte_buffer_size;
    int i = 0;
    int result;
    int end_of_stream = 0;

    result = avformat_open_input(&fmt_ctx, filename, NULL, NULL);
    if (result < 0) {
        LOGE("%s", "Can't open file\n");
        return result;
    }

    result = avformat_find_stream_info(fmt_ctx, NULL);
    if (result < 0) {
        LOGE("%s", "Can't get stream info\n");
        return result;
    }

    video_stream = av_find_best_stream(fmt_ctx, AVMEDIA_TYPE_VIDEO, -1, -1, NULL, 0);
    if (video_stream < 0) {
        LOGE("%s", "Can't find video stream in input file\n");
        return -1;
    }

    origin_par = fmt_ctx->streams[video_stream]->codecpar;

    codec = avcodec_find_decoder(origin_par->codec_id);
    if (!codec) {
        LOGE("%s", "Can't find decoder\n");
        return -1;
    }

    ctx = avcodec_alloc_context3(codec);
    if (!ctx) {
        LOGE("%s", "Can't allocate decoder context\n");
        return AVERROR(ENOMEM);
    }

    result = avcodec_parameters_to_context(ctx, origin_par);
    if (result) {
        LOGE("%s", "Can't copy decoder context\n");
        return result;
    }

    result = avcodec_open2(ctx, codec, NULL);
    if (result < 0) {
        LOGE("%s", "Can't open decoder\n");
        return result;
    }

    fr = av_frame_alloc();
    if (!fr) {
        LOGE("%s", "Can't allocate frame\n");
        return AVERROR(ENOMEM);
    }

    byte_buffer_size = av_image_get_buffer_size(ctx->pix_fmt, ctx->width, ctx->height, 16);
    byte_buffer = av_malloc(byte_buffer_size);
    if (!byte_buffer) {
        LOGE("%s", "Can't allocate buffer\n");
        return AVERROR(ENOMEM);
    }

    printf("#tb %d: %d/%d\n", video_stream, fmt_ctx->streams[video_stream]->time_base.num,
           fmt_ctx->streams[video_stream]->time_base.den);
    i = 0;
    av_init_packet(&pkt);
    do {
        if (!end_of_stream)
            if (av_read_frame(fmt_ctx, &pkt) < 0)
                end_of_stream = 1;
        if (end_of_stream) {
            pkt.data = NULL;
            pkt.size = 0;
        }
        if (pkt.stream_index == video_stream || end_of_stream) {
            got_frame = 0;
            if (pkt.pts == AV_NOPTS_VALUE)
                pkt.pts = pkt.dts = i;
            result = avcodec_decode_video2(ctx, fr, &got_frame, &pkt);
            if (result < 0) {
                LOGE(NULL, AV_LOG_ERROR, "Error decoding frame\n");
                return result;
            }
            if (got_frame) {
                number_of_written_bytes = av_image_copy_to_buffer(byte_buffer, byte_buffer_size,
                                                                  (const uint8_t *const *) fr->data,
                                                                  (const int *) fr->linesize,
                                                                  ctx->pix_fmt, ctx->width,
                                                                  ctx->height, 1);
                if (number_of_written_bytes < 0) {
                    LOGE("%s", "Can't copy image to buffer\n");
                    return number_of_written_bytes;
                }
                LOGD("%d, %10"
                             PRId64
                             ", %10"
                             PRId64
                             ", %8"
                             PRId64
                             ", %8d, 0x%08lx\n", video_stream,
                     fr->pts, fr->pkt_dts, fr->pkt_duration,
                     number_of_written_bytes, av_adler32_update(0, (const uint8_t *) byte_buffer,
                                                                number_of_written_bytes));
            }
            av_packet_unref(&pkt);
            av_init_packet(&pkt);
        }
        i++;
    } while (!end_of_stream || got_frame);

    av_packet_unref(&pkt);
    av_frame_free(&fr);
    avcodec_close(ctx);
    avformat_close_input(&fmt_ctx);
    avcodec_free_context(&ctx);
    av_freep(&byte_buffer);
    return 0;
}

JNIEXPORT void JNICALL Java_top_andnux_ffmepgdemo_FFmpeg_decode(JNIEnv *env,
                                                                jclass clazz, jstring input_jstr,
                                                                jobject surface) {
    const char *input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
//    video_decode(input_cstr);
    //1 注册组件
    av_register_all();
    AVFormatContext *pFormatCtx = avformat_alloc_context();
    // 打开视频
    if (avformat_open_input(&pFormatCtx, input_cstr, NULL, NULL) != 0) {
        LOGE("%s", "打开视频文件失败！");
        return;
    }
    //查找视频信息
    if (avformat_find_stream_info(pFormatCtx, NULL) < 0) {
        LOGE("%s", "无法获取输入文件信息");
        return;
    }
    //3.找到视频流
    int video_stream_index = -1;
    int audio_stream_index = -1;
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
        }
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_AUDIO) {
            audio_stream_index = i;
        }
    }
    if (video_stream_index == -1) {
        LOGE("%s", "没有视频流信息");
        return;
    }
    if (audio_stream_index == -1) {
        LOGE("%s", "没有音频流信息");
        return;
    }
    AVCodecContext *pVideoCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
    //4.拿到解码器
    AVCodec *videoCodec = avcodec_find_decoder(pVideoCodecCtx->codec_id);
    if (videoCodec == NULL) {
        LOGE("%s", "没有找到视频解码器");
        return;
    }
    // 5.打开解码器
    if (avcodec_open2(pVideoCodecCtx, videoCodec, NULL) < 0) {
        LOGE("%s", "视频解码器无法打开");
        return;
    }

    //6.一针一针读取压缩的视频数据AVPacket
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *yuv_frame = av_frame_alloc();
    AVFrame *rgb_frame = av_frame_alloc();

    int got_frame, index = 0, ret;
    //原生绘制：1窗体
    ANativeWindow* nativeWindow = ANativeWindow_fromSurface(env,surface);
    //原生绘制：2绘制时的缓冲区
    ANativeWindow_Buffer outBuffer;
    //原生绘制：2设置缓冲区的属性（宽、高、像素格式）
    ANativeWindow_setBuffersGeometry(nativeWindow, pVideoCodecCtx->width,
                                     pVideoCodecCtx->height,WINDOW_FORMAT_RGBA_8888);

    while (0 <= av_read_frame(pFormatCtx, pPacket)) {
        if (pPacket->stream_index == video_stream_index) {
            ret = avcodec_decode_video2(pVideoCodecCtx, yuv_frame, &got_frame, pPacket);
            if (ret < 0) {
                LOGD("%s", "解码完成");
            }
            //解码一帧成功
            if (got_frame > 0) {
                LOGD("解码：%d", index++);
                //原生绘制：4锁定画布
                ANativeWindow_lock(nativeWindow,&outBuffer,NULL);
                //设置rgb_frame的属性（像素格式、宽高）和缓冲区
                //rgb_frame缓冲区与outBuffer.bits是同一块内存
                avpicture_fill((AVPicture *)rgb_frame, outBuffer.bits, AV_PIX_FMT_RGBA,
                               pVideoCodecCtx->width, pVideoCodecCtx->height);
                //YUV->RGBA_8888
                I420ToARGB(yuv_frame->data[0],yuv_frame->linesize[0],
                           yuv_frame->data[2],yuv_frame->linesize[2],
                           yuv_frame->data[1],yuv_frame->linesize[1],
                           rgb_frame->data[0], rgb_frame->linesize[0],
                           pVideoCodecCtx->width,pVideoCodecCtx->height);
                //原生绘制：6 unlock
                ANativeWindow_unlockAndPost(nativeWindow);

                usleep(1000 * 16);
            }
        }
        av_free_packet(pPacket);
    }
    ANativeWindow_release(nativeWindow);
    avformat_free_context(pFormatCtx);
    av_frame_free(&yuv_frame);
    av_frame_free(&rgb_frame);
    avcodec_close(pVideoCodecCtx);
    (*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);
}


#pragma clang diagnostic pop
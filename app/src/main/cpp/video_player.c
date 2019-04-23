#include<jni.h>
#include <android/log.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "libavutil/adler32.h"
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libswscale/swscale.h"
#include "libavutil/imgutils.h"

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
                                                                jstring output_jstr) {
    const char *input_cstr = (*env)->GetStringUTFChars(env, input_jstr, NULL);
    const char *output_cstr = (*env)->GetStringUTFChars(env, output_jstr, NULL);
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
    for (int i = 0; i < pFormatCtx->nb_streams; ++i) {
        if (pFormatCtx->streams[i]->codec->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream_index = i;
            break;
        }
    }
    if (video_stream_index == -1) {
        LOGE("%s", "没有视频流信息");
        return;
    }
    AVCodecContext *pCodecCtx = pFormatCtx->streams[video_stream_index]->codec;
    //4.拿到解码器
    AVCodec *codec = avcodec_find_decoder(pCodecCtx->codec_id);
    if (codec == NULL) {
        LOGE("%s", "没有找到解码器");
        return;
    }
    // 5.打开解码器
    if (avcodec_open2(pCodecCtx, codec, NULL) < 0) {
        LOGE("%s", "解码器无法打开");
        return;
    }

    //6.一针一针读取压缩的视频数据AVPacket
    AVPacket *pPacket = av_packet_alloc();
    AVFrame *pFrame = av_frame_alloc();
    AVFrame *yuvFrame = av_frame_alloc();

    //指定缓冲区
    uint8_t *out_buffer = av_malloc(
            (size_t) avpicture_get_size(AV_PIX_FMT_YUV420P, pCodecCtx->width, pCodecCtx->height));
    avpicture_fill((AVPicture *) yuvFrame, out_buffer, AV_PIX_FMT_YUV420P,
                   pCodecCtx->width, pCodecCtx->height);

    int got_frame, index = 0, ret;
    // 打开文件
    FILE *file = fopen(output_cstr, "wb");
    //像素格式转换
    struct SwsContext *swsContext = sws_getContext(pCodecCtx->width, pCodecCtx->height,
                                                   pCodecCtx->pix_fmt, pCodecCtx->width,
                                                   pCodecCtx->height,
                                                   AV_PIX_FMT_YUV420P, SWS_FAST_BILINEAR, NULL,
                                                   NULL, NULL);
//    ret = av_read_frame(pFormatCtx, pPacket);
//    LOGE("%d",ret);
//    ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, pPacket);
//    LOGE("=====%d",ret);
    while (0 <= av_read_frame(pFormatCtx, pPacket)) {
        if (pPacket->stream_index == video_stream_index) {
            ret = avcodec_decode_video2(pCodecCtx, pFrame, &got_frame, pPacket);
            if (ret < 0) {
                LOGD("%s", "解码完成");
            }
            //解码一帧成功
            if (got_frame > 0) {
                LOGD("解码：%d", index++);
                // 转为指定的yuv420p
                sws_scale(swsContext, pFrame->data, pFrame->linesize, 0,
                      pCodecCtx->height, yuvFrame->data, yuvFrame->linesize);
                int y_size = pFrame->width * pFrame->height;
                fwrite(yuvFrame->data[0], sizeof(uint8_t), (size_t) y_size, file);
                fwrite(yuvFrame->data[1], sizeof(uint8_t), (size_t) y_size / 4, file);
                fwrite(yuvFrame->data[2], sizeof(uint8_t), (size_t) y_size / 4, file);
            }
        }
        av_free_packet(pPacket);
    }
    fclose(file);
    av_frame_free(&pFrame);
    avcodec_close(pCodecCtx);
    (*env)->ReleaseStringUTFChars(env, input_jstr, input_cstr);
    (*env)->ReleaseStringUTFChars(env, output_jstr, output_cstr);
}

#pragma clang diagnostic pop
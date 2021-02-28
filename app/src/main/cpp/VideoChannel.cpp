//
// Created by Administrator on 2019/11/21.
//


#include "VideoChannel.h"


extern "C" {
#include <libswscale/swscale.h>
#include <libavutil/rational.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#define AV_SYNC_THRESHOLD_MIN 0.04
#define AV_SYNC_THRESHOLD_MAX 0.1

VideoChannel::VideoChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                           const AVRational &base, int fps) :
        BaseChannel(channelId, helper, avCodecContext, base), fps(fps) {

    pthread_mutex_init(&surfaceMutex, 0);

}

VideoChannel::~VideoChannel() {
    pthread_mutex_destroy(&surfaceMutex);
}

void *videoDecode_t(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->decode();
    return 0;
}

void *videoPlay_t(void *args) {
    VideoChannel *videoChannel = static_cast<VideoChannel *>(args);
    videoChannel->_play();
    return 0;
}


void VideoChannel::_play() {
    // 缩放、格式转换
    SwsContext *swsContext = sws_getContext(avCodecContext->width,
                                            avCodecContext->height,
                                            avCodecContext->pix_fmt,
                                            avCodecContext->width,
                                            avCodecContext->height,
                                            AV_PIX_FMT_RGBA, SWS_FAST_BILINEAR,
                                            0, 0, 0);

    uint8_t *data[4];
    int linesize[4];
    av_image_alloc(data, linesize, avCodecContext->width, avCodecContext->height,
                   AV_PIX_FMT_RGBA, 1);

    AVFrame *frame = 0;  //100x100 ,window : 100x100
    double frame_delay = 1.0 / fps;
    int ret;
    while (isPlaying) {

        //阻塞方法
        ret = frame_queue.deQueue(frame);
        // 阻塞完了之后可能用户已经停止播放
        if (!isPlaying) {
            break;
        }

        if (!ret) {
            continue;
        }
        double extra_delay = frame->repeat_pict / (2.0 * fps);
        double delay = frame_delay + extra_delay;
        if (audioChannel) {
            // best_effort_timestamp在大多数时间和pts一样, 但有特殊情况
            clock = frame->best_effort_timestamp * av_q2d(time_base);
            double diff = clock - audioChannel->clock;
            // 给一个允许的时间差范围0.04 ~ 0.1 参考ffplay
            double sync = FFMAX(AV_SYNC_THRESHOLD_MIN, FFMIN(AV_SYNC_THRESHOLD_MAX, delay));

            if (diff <= -sync) {
                // 修改delay变小些 快点赶上音频
                delay = FFMAX(0, delay + diff);
            } else if (diff > sync) {
                // 视频太快 delay应该加大 等等音频
                delay += diff;
            }
        }

        av_usleep((int) (delay * 1000000));

        //todo 2、指针数据，比如RGBA，每一个维度的数据就是一个指针，那么RGBA需要4个指针，所以就是4个元素的数组，数组的元素就是指针，指针数据
        // 3、每一行数据他的数据个数
        // 4、 offset
        // 5、 要转化图像的高
        sws_scale(swsContext, frame->data, frame->linesize, 0,
                  frame->height, data, linesize);
        //画画
        onDraw(data, linesize, avCodecContext->width, avCodecContext->height);
        releaseAvFrame(frame);
    }
    av_free(&data[0]);
    isPlaying = false;
    sws_freeContext(swsContext);
}

void VideoChannel::onDraw(uint8_t **data, int *linesize, int width, int height) {
    pthread_mutex_lock(&surfaceMutex);
    if (!window) {
        pthread_mutex_unlock(&surfaceMutex);
        return;
    }
    ANativeWindow_setBuffersGeometry(window, width, height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer buffer;
    if (ANativeWindow_lock(window, &buffer, 0) != 0) {
        ANativeWindow_release(window);
        window = 0;
        pthread_mutex_unlock(&surfaceMutex);
        return;
    }
    //把视频数据刷到buffer中
    uint8_t *dstData = static_cast<uint8_t *>(buffer.bits);
    int dstSize = buffer.stride * 4;
    // 视频图像的rgba数据
    uint8_t *srcData = data[0];
    int srcSize = linesize[0];

    //一行一行拷贝
    for (int i = 0; i < buffer.height; ++i) {
        memcpy(dstData + i * dstSize, srcData + i * srcSize, srcSize);
    }

    ANativeWindow_unlockAndPost(window);
    pthread_mutex_unlock(&surfaceMutex);
}

void VideoChannel::play() {
    isPlaying = 1;
    // 开启队列的工作
    setEnable(true);
    //解码
    pthread_create(&videoDecodeTask, 0, videoDecode_t, this);
    //播放
    pthread_create(&videoPlayTask, 0, videoPlay_t, this);
}

void VideoChannel::decode() {
    AVPacket *packet = 0;
    while (isPlaying) {
        //阻塞
        // 1、能够取到数据
        // 2、停止播放
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        // 向解码器发送解码数据
        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(packet);
        if (ret < 0) {
            break;
        }

        //从解码器取出解码好的数据
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (ret == AVERROR(EAGAIN)) { //我还要
            continue;
        } else if (ret < 0) {
            break;
        }
        frame_queue.enQueue(frame);
    }
    releaseAvPacket(packet);
}


void VideoChannel::setWindow(ANativeWindow *window) {
    pthread_mutex_lock(&surfaceMutex);
    if (this->window) {
        ANativeWindow_release(this->window);
    }
    this->window = window;
    pthread_mutex_unlock(&surfaceMutex);
}

void VideoChannel::stop() {
    isPlaying = false;
    helper = nullptr;
    setEnable(0);
    pthread_join(videoDecodeTask, nullptr);
    pthread_join(videoPlayTask, nullptr);
    if (window) {
        ANativeWindow_release(window);
        window = nullptr;
    }
}



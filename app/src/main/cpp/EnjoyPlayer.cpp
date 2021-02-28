//
// Created by Administrator on 2019/11/19.
//

#include <cstring>
#include <malloc.h>
#include "Log.h"

#include "EnjoyPlayer.h"

extern "C" {
#include <libavformat/avformat.h>
}

void *prepare_t(void *args) {
    EnjoyPlayer *player = static_cast<EnjoyPlayer *>(args);
    player->_prepare();
    return 0;
}

EnjoyPlayer::EnjoyPlayer(JavaCallHelper *helper) : helper(helper) {
    avformat_network_init();
    videoChannel = nullptr;
}

EnjoyPlayer::~EnjoyPlayer() {
    avformat_network_deinit();
    delete helper;
    helper = nullptr;
    if (path) {
        delete[] path;
        path = nullptr;
    }
}

void EnjoyPlayer::setDataSource(const char *path_) {
//    path  = static_cast< char *>(malloc(strlen(path_) + 1));
//    memset((void *) path, 0, strlen(path) + 1);
//
//    memcpy(path,path_,strlen(path_));
    path = new char[strlen(path_) + 1];
    strcpy(path, path_);
}

void EnjoyPlayer::prepare() {
    //解析  耗时！
    pthread_create(&prepareTask, 0, prepare_t, this);
}

void EnjoyPlayer::_prepare() {
    avFormatContext = avformat_alloc_context();
    /*
     * 1、打开媒体问题
     */
    //参数3： 输入文件的封装格式 ，传null表示 自动检测格式。 avi / flv
    //参数4： map集合，比如打开网络文件超时设置
//    AVDictionary *opts;
//    av_dict_set(&opts, "timeout", "3000000", 0);
    int ret = avformat_open_input(&avFormatContext, path, 0, 0);
    if (ret != 0) {
        //.....
        LOGE("打开%s 失败，返回:%d 错误描述:%s", path, ret, av_err2str(ret));
        helper->onError(FFMPEG_CAN_NOT_OPEN_URL, THREAD_CHILD);
        return;
    }


    /**
     * 2、查找媒体流
     */
    ret = avformat_find_stream_info(avFormatContext, 0);
    if (ret < 0) {
        LOGE("查找媒体流 %s 失败，返回:%d 错误描述:%s", path, ret, av_err2str(ret));
        helper->onError(FFMPEG_CAN_NOT_FIND_STREAMS, THREAD_CHILD);
        return;
    }
    // 得到视频时长，单位是秒
    duration = avFormatContext->duration / AV_TIME_BASE;
    // 这个媒体文件中有几个媒体流 (视频流、音频流)
    for (int i = 0; i < avFormatContext->nb_streams; ++i) {
        AVStream *avStream = avFormatContext->streams[i];
        // 解码信息
        AVCodecParameters *parameters = avStream->codecpar;
        //查找解码器
        AVCodec *dec = avcodec_find_decoder(parameters->codec_id);
        if (!dec) {
            helper->onError(FFMPEG_FIND_DECODER_FAIL, THREAD_CHILD);
            return;
        }

        AVCodecContext *codecContext = avcodec_alloc_context3(dec);
        // 把解码信息赋值给了解码上下文中的各种成员
        if (avcodec_parameters_to_context(codecContext, parameters) < 0) {
            helper->onError(FFMPEG_CODEC_CONTEXT_PARAMETERS_FAIL, THREAD_CHILD);
            return;
        }
        // 打开解码器
        if (avcodec_open2(codecContext, dec, 0) != 0) {
            //打开失败
            helper->onError(FFMPEG_OPEN_DECODER_FAIL, THREAD_CHILD);
            return;
        }


        if (parameters->codec_type == AVMEDIA_TYPE_AUDIO) {
            audioChannel = new AudioChannel(i, helper, codecContext, avStream->time_base);
        } else if (parameters->codec_type == AVMEDIA_TYPE_VIDEO) {
            // 帧率
            double fps = av_q2d(avStream->avg_frame_rate);
            if (isnan(fps) || fps == 0) {
                fps = av_q2d(avStream->r_frame_rate);
            }
            if (isnan(fps) || fps == 0) {
                fps = av_q2d(av_guess_frame_rate(avFormatContext, avStream, nullptr));
            }
            videoChannel = new VideoChannel(
                    i, helper, codecContext, avStream->time_base, (int) fps);
            videoChannel->setWindow(window);
        }
    }

    if (!videoChannel && !audioChannel) {
        helper->onError(FFMPEG_NOMEDIA, THREAD_CHILD);
        return;
    }

    //告诉java准备好了，可以播放了
    helper->onParpare(THREAD_CHILD);

}

void *start_t(void *args) {
    EnjoyPlayer *player = static_cast<EnjoyPlayer *>(args);
    player->_start();
    return 0;
}

void EnjoyPlayer::start() {
    //1、读取媒体源的数据
    //2、根据数据类型放入Audio/VideoChannel的队列中
    isPlaying = 1;
    if (videoChannel) {
        videoChannel->audioChannel = audioChannel;
        videoChannel->play();
    }
    if (audioChannel) {
        audioChannel->play();
    }
    pthread_create(&startTask, 0, start_t, this);
}

void EnjoyPlayer::_start() {
    int ret;
    while (isPlaying) {
        AVPacket *packet = av_packet_alloc();
        ret = av_read_frame(avFormatContext, packet);
        if (ret == 0) {
            if (videoChannel && packet->stream_index == videoChannel->channelId) {
                videoChannel->pkt_queue.enQueue(packet);
            } else if (audioChannel && packet->stream_index == audioChannel->channelId) {
                audioChannel->pkt_queue.enQueue(packet);
            } else {
                av_packet_free(&packet);
            }
        } else if (ret == AVERROR_EOF) { //end of file
            //读取完毕，不一定播放完毕
            if (videoChannel->pkt_queue.empty() && videoChannel->frame_queue.empty()) {
                //播放完毕
                break;
            }
        } else {
            break;
        }
    }

    isPlaying = false;
    videoChannel->stop();
    audioChannel->stop();
}

void EnjoyPlayer::setWindow(ANativeWindow *window) {
    this->window = window;
    if (videoChannel) {
        videoChannel->setWindow(window);
    }
}

void EnjoyPlayer::stop() {
    isPlaying = false;
    // 等待线程执行结束
    pthread_join(prepareTask, nullptr);
    pthread_join(startTask, nullptr);
    release();
}

void EnjoyPlayer::release() {
    if (audioChannel) {
        delete audioChannel;
        audioChannel = nullptr;
    }
    if (videoChannel) {
        delete videoChannel;
        videoChannel = nullptr;
    }
    if (avFormatContext) {
        avformat_close_input(&avFormatContext);
        avformat_free_context(avFormatContext);
        avFormatContext = nullptr;
    }
}

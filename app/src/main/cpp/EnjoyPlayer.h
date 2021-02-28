//
// Created by Administrator on 2019/11/19.
//

#ifndef ENJOYPLAYER_ENJOYPLAYER_H
#define ENJOYPLAYER_ENJOYPLAYER_H

#include <pthread.h>
#include <android/native_window_jni.h>

#include "JavaCallHelper.h"
#include "VideoChannel.h"
#include "AudioChannel.h"

extern "C" {
#include <libavformat/avformat.h>
};

class EnjoyPlayer {
    friend void *prepare_t(void *args);

    friend void *start_t(void *args);

public:
    EnjoyPlayer(JavaCallHelper *helper);

    ~EnjoyPlayer();

    void start();

    void stop();

    void release();

    void setWindow(ANativeWindow *window);

public:
    void setDataSource(const char *path);

    void prepare();

private:
    void _prepare();

    void _start();

private:
    char *path;
    pthread_t prepareTask;
    JavaCallHelper *helper;
    int64_t duration;
    VideoChannel *videoChannel;
    AudioChannel *audioChannel;

    pthread_t startTask;
    bool isPlaying;
    AVFormatContext *avFormatContext;

    ANativeWindow *window = 0;
};


#endif //ENJOYPLAYER_ENJOYPLAYER_H

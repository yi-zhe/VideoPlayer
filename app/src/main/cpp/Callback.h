//
// Created by liuyang on 2021/3/4.
//

#ifndef VIDEOPLAYER_CALLBACK_H
#define VIDEOPLAYER_CALLBACK_H
extern "C" {
#include "include/librtmp/rtmp.h"
};

typedef void(*Callback)(RTMPPacket *p);

#endif //VIDEOPLAYER_CALLBACK_H

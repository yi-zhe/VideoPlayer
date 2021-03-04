//
// Created by liuyang on 2021/3/3.
//

#ifndef VIDEOPLAYER_VIDEOCHANNELWITHX264_H
#define VIDEOPLAYER_VIDEOCHANNELWITHX264_H

#include "Callback.h"

extern "C" {
#include <stdint.h>
#include <x264.h>
}

class VideoChannel2 {
public:
    VideoChannel2();

    ~VideoChannel2();

    void encode(uint8_t *data);

public:
    void openCodec(int w, int h, int fps, int bitrate);

    void setCallback(Callback callback);

    void resetPts(){
        i_pts = 0;
    };

private:
    x264_t *codec = nullptr;
    int ySize = 0;
    int uvSize = 0;
    int width = 0;
    int height = 0;
    int64_t i_pts = 0;

    Callback callback = nullptr;

    void sendVideoConfig(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len);

    void sendFrame(int type, uint8_t *payload, int payload1);
};

#endif //VIDEOPLAYER_VIDEOCHANNELWITHX264_H

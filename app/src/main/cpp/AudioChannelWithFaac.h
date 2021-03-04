//
// Created by liuyang on 2021/3/4.
//

#ifndef VIDEOPLAYER_AUDIOCHANNELWITHFAAC_H
#define VIDEOPLAYER_AUDIOCHANNELWITHFAAC_H

#include <cstdlib>
#include <cstring>
#include "Callback.h"

extern "C" {
#include <faac.h>
#include <librtmp/rtmp.h>
}

class AudioChannel2 {
public:
    AudioChannel2();

    ~AudioChannel2();

    void encode(int32_t *data, int len);

public:
    void openCodec(int sampleRate, int channels);

    int getInputByteNum() {
        return inputByteNum;
    }

    void closeCodec();

    void setCallback(Callback callback) {
        this->callback = callback;
    }

    RTMPPacket *getAudioConfig();

private:
    faacEncHandle codec = nullptr;
    unsigned long inputByteNum = 0;
    unsigned long maxOutputBytes;
    unsigned char *outputBuffer = nullptr;
    Callback callback;
};

#endif //VIDEOPLAYER_AUDIOCHANNELWITHFAAC_H

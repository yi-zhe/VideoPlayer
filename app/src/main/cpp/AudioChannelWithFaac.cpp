//
// Created by liuyang on 2021/3/4.
//

#include "AudioChannelWithFaac.h"
#include "Callback.h"

AudioChannel2::AudioChannel2() {

}

AudioChannel2::~AudioChannel2() {
    closeCodec();
    free(outputBuffer);
    outputBuffer = nullptr;
}

void AudioChannel2::openCodec(int sampleRate, int channels) {
    // 表示输入样本: 要送给编码器的样本数
    unsigned long inSamples;
    codec = faacEncOpen(sampleRate, channels, &inSamples, &maxOutputBytes);

    outputBuffer = static_cast<unsigned char *>(malloc(maxOutputBytes));

    // 样本是16位的 每个样本是两个字节
    inputByteNum = inSamples * 2;

    faacEncConfiguration *configuration = faacEncGetCurrentConfiguration(codec);
    configuration->mpegVersion = MPEG4;
    configuration->aacObjectType = LOW;
    // 0 AAC裸数据 1 每一帧音频编码数据都携带ADTS(采样 声道信息的数据头)
    configuration->outputFormat = 1;
    configuration->inputFormat = FAAC_INPUT_16BIT;
    faacEncSetConfiguration(codec, configuration);
}

void AudioChannel2::closeCodec() {
    if (codec) {
        faacEncClose(codec);
        codec = nullptr;
    }
}

void AudioChannel2::encode(int32_t *data, int len) {
    int byteLen = faacEncEncode(codec, data, len, outputBuffer, maxOutputBytes);
    if (byteLen > 0) {
        RTMPPacket *packet = new RTMPPacket;
        RTMPPacket_Alloc(packet, byteLen + 2);
        packet->m_body[0] = 0xAF;
        packet->m_body[1] = 0x01;
        memcpy(&packet->m_body[2], outputBuffer, byteLen);
        packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
        packet->m_nBodySize = byteLen + 2;
        packet->m_nChannel = 0x11;
        packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
        callback(packet);
    }
}

RTMPPacket *AudioChannel2::getAudioConfig() {
    u_char *buf;
    u_long len;
    faacEncGetDecoderSpecificInfo(codec, &buf, &len);
    RTMPPacket *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, len + 2);
    packet->m_body[0] = 0xAF;
    packet->m_body[1] = 0x00;
    memcpy(&packet->m_body[2], buf, len);
    packet->m_packetType = RTMP_PACKET_TYPE_AUDIO;
    packet->m_nBodySize = len + 2;
    packet->m_nChannel = 0x11;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    return packet;
}

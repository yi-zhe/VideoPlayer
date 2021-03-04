//
// Created by liuyang on 2021/3/3.
//

#include <cstring>
#include <alloca.h>
#include <librtmp/rtmp.h>
#include "VideoChannelWithX264.h"

VideoChannel2::VideoChannel2() {

}

void VideoChannel2::openCodec(int w, int h, int fps, int bitrate) {
    x264_param_t param;
    x264_param_default_preset(&param, "ultrafast", "zerolatency");
    // 没有B帧
    param.i_level_idc = 32;
    param.i_csp = X264_CSP_I420;
    param.i_width = w;
    param.i_height = h;
    // 无B帧
    param.i_bframe = 0;
    param.rc.i_rc_method = X264_RC_ABR;
    param.rc.i_bitrate = bitrate / 1000;
    param.rc.i_vbv_max_bitrate = bitrate / 1000 * 1.2;
    param.i_fps_num = fps;
    param.i_fps_den = 1;
    param.i_keyint_max = fps * 2;
    param.b_repeat_headers = 1;
    param.i_threads = 1;
    param.rc.i_lookahead = 0;
    x264_param_apply_profile(&param, "baseline");
    codec = x264_encoder_open(&param);
    ySize = w * h;
    uvSize = ySize >> 2;
    this->width = w;
    this->height = h;
}

VideoChannel2::~VideoChannel2() {
    if (codec) {
        x264_encoder_close(codec);
        codec = nullptr;
    }
}

void VideoChannel2::encode(uint8_t *data) {
    x264_picture_t pic_in;
    x264_picture_t pic_out;
    x264_picture_alloc(&pic_in, X264_CSP_I420, width, height);
    pic_in.img.plane[0] = data;
    pic_in.img.plane[1] = data + ySize;
    pic_in.img.plane[2] = data + ySize + uvSize;

    // 编码的i_pts每次需要增长 它可以当做图片编号
    pic_in.i_pts = i_pts++;

    x264_nal_t *pp_nal = nullptr;
    // 输出了多少个帧 也就是pp_nal数组中元素的个数
    int pi_nal;
    int ret = x264_encoder_encode(codec, &pp_nal, &pi_nal, &pic_in, &pic_out);
    if (ret <= 0) {
        return;
    }

    int sps_len, pps_len;
    uint8_t *sps, *pps;
    for (int i = 0; i < pi_nal; i++) {
        int type = pp_nal[i].i_type;
        int i_payload = pp_nal[i].i_payload;
        uint8_t *payload = pp_nal[i].p_payload;
        if (NAL_SPS == type) {
            // sps后面肯定是pps
            sps_len = i_payload - 4; // 去掉开头标志00000001
            sps = (uint8_t *) alloca(sps_len); // 栈中申请内存
            memcpy(sps, payload + 4, sps_len);
        } else if (NAL_PPS == type) {
            pps_len = i_payload - 4; // 去掉开头标志00000001
            pps = (uint8_t *) alloca(pps_len);
            memcpy(pps, payload + 4, pps_len);

            // pps后肯定有I帧 发送I帧前, 要发一个sps pps
            sendVideoConfig(sps, pps, sps_len, pps_len);
        } else {
            sendFrame(type, payload, i_payload);
        }
    }
}

void VideoChannel2::sendVideoConfig(uint8_t *sps, uint8_t *pps, int sps_len, int pps_len) {
    int bodySize = sps_len + 13 + pps_len + 3;
    auto *packet = new RTMPPacket;
    RTMPPacket_Alloc(packet, bodySize);
    int index = 0;
    // 固定头
    packet->m_body[index++] = 0x17;
    // 类型
    packet->m_body[index++] = 0x00;
    packet->m_body[index++] = 0x00;
    packet->m_body[index++] = 0x00;
    packet->m_body[index++] = 0x00;
    // 版本
    packet->m_body[index++] = 0x01;
    // 编码规格
    packet->m_body[index++] = sps[1];
    packet->m_body[index++] = sps[2];
    packet->m_body[index++] = sps[3];
    packet->m_body[index++] = 0xFF;
    // sps
    packet->m_body[index++] = 0xE1;
    // sps长度
    packet->m_body[index++] = (sps_len >> 8) & 0xFF;
    packet->m_body[index++] = (sps_len) & 0xFF;
    memcpy(&packet->m_body[index], sps, sps_len);
    index += sps_len;

    // pps
    packet->m_body[index++] = 0x01;
    packet->m_body[index++] = (pps_len >> 8) & 0xFF;
    packet->m_body[index++] = (pps_len) & 0xFF;
    memcpy(&packet->m_body[index], pps, pps_len);

    // 视频
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nBodySize = bodySize;
    packet->m_nChannel = 0x10;
    // 对于sps pps该值没有啥意义
    packet->m_nTimeStamp = 0;
    packet->m_hasAbsTimestamp = 0;
    packet->m_headerType = RTMP_PACKET_SIZE_MEDIUM;
    callback(packet);
}

void VideoChannel2::setCallback(Callback callback) {
    this->callback = callback;
}

void VideoChannel2::sendFrame(int type, uint8_t *p_payload, int i_payload) {
//去掉 00 00 00 01 / 00 00 01
    if (p_payload[2] == 0x00) {
        i_payload -= 4;
        p_payload += 4;
    } else if (p_payload[2] == 0x01) {
        i_payload -= 3;
        p_payload += 3;
    }
    RTMPPacket *packet = new RTMPPacket;
    int bodySize = 9 + i_payload;
    RTMPPacket_Alloc(packet, bodySize);
    RTMPPacket_Reset(packet);
//    int type = payload[0] & 0x1f;
    packet->m_body[0] = 0x27;
    //关键帧
    if (type == NAL_SLICE_IDR) {
        packet->m_body[0] = 0x17;
    }
    //类型
    packet->m_body[1] = 0x01;
    //时间戳
    packet->m_body[2] = 0x00;
    packet->m_body[3] = 0x00;
    packet->m_body[4] = 0x00;
    //数据长度 int 4个字节 相当于把int转成4个字节的byte数组
    packet->m_body[5] = (i_payload >> 24) & 0xff;
    packet->m_body[6] = (i_payload >> 16) & 0xff;
    packet->m_body[7] = (i_payload >> 8) & 0xff;
    packet->m_body[8] = (i_payload) & 0xff;

    //图片数据
    memcpy(&packet->m_body[9], p_payload, i_payload);

    packet->m_hasAbsTimestamp = 0;
    packet->m_nBodySize = bodySize;
    packet->m_packetType = RTMP_PACKET_TYPE_VIDEO;
    packet->m_nChannel = 0x10;
    packet->m_headerType = RTMP_PACKET_SIZE_LARGE;
    callback(packet);
}

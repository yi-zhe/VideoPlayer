#include <jni.h>
#include <string>
#include <pthread.h>
#include "EnjoyPlayer.h"
#include "JavaCallHelper.h"
#include "Log.h"
#include "VideoChannelWithX264.h"
#include "AudioChannelWithFaac.h"

extern "C" {
#include <librtmp/rtmp.h>
#include "packt.h"
}
JavaVM *javaVM = 0;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_tools_player_EnjoyPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    EnjoyPlayer *player = new EnjoyPlayer(new JavaCallHelper(javaVM, env, thiz));
    return (jlong)
            player;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_setDataSource(JNIEnv *env, jobject thiz, jlong nativeHandle,
                                                jstring path_) {
    const char *path = env->GetStringUTFChars(path_, 0);
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->setDataSource(path);
    env->ReleaseStringUTFChars(path_, path);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_prepare(JNIEnv *env, jobject thiz, jlong nativeHandle) {
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->prepare();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_start(JNIEnv *env, jobject thiz, jlong nativeHandle) {
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->start();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_setSurface(JNIEnv *env, jobject thiz, jlong nativeHandle,
                                             jobject surface) {
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    player->setWindow(window);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_stop(JNIEnv *env, jobject thiz, jlong nativeHandle) {
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->stop();
    delete player;
}

Live *live = nullptr;
RTMP *rtmp = nullptr;

int sendVideo(jbyte *data, jint len, jlong tms);

int sendAudio(jbyte *data, jint len, jlong tms, jint type);

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_tools_live_ScreenLive_connect(JNIEnv *env, jobject thiz, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, nullptr);
    int ret;
    do {
        live = (Live *) malloc(sizeof(Live));
        memset(live, 0, sizeof(Live));
        live->rtmp = RTMP_Alloc();
        RTMP_Init(live->rtmp);
        live->rtmp->Link.timeout = 10;
        LOGI("connect %s", url);
        if (!(ret = RTMP_SetupURL(live->rtmp, (char *) url))) break;
        RTMP_EnableWrite(live->rtmp);
        LOGI("RTMP_Connect");
        if (!(ret = RTMP_Connect(live->rtmp, 0))) break;
        LOGI("RTMP_ConnectStream ");
        if (!(ret = RTMP_ConnectStream(live->rtmp, 0))) break;
        LOGI("connect success");
    } while (0);


    if (!ret && live) {
        free(live);
        live = nullptr;
    }
    env->ReleaseStringUTFChars(url_, url);
    return ret;
}


int sendPacket(RTMPPacket *packet) {
    int r = RTMP_SendPacket(live->rtmp, packet, 1);
    RTMPPacket_Free(packet);
    free(packet);
    return r;
}

int sendVideo(jbyte *buf, jint len, jlong tms) {
    int ret;
    do {
        if (buf[4] == 0x67) {//sps pps
            if (live && (!live->pps || !live->sps)) {
                prepareVideo(buf, len, live);
            }
        } else {
            if (buf[4] == 0x65) {//关键帧
                RTMPPacket *packet = createVideoPackage(live);
                if (!(ret = sendPacket(packet))) {
                    break;
                }
            }
            //将编码之后的数据 按照 flv、rtmp的格式 拼好之后
            RTMPPacket *packet = createVideoPackage2(buf, len, tms, live);
            ret = sendPacket(packet);
        }
    } while (0);
    return ret;
}

int sendAudio(jbyte *buf, jint len, jlong tms, jint type) {
    int ret;
    do {
        RTMPPacket *packet = createAudioPacket(buf, len, type, tms, live);
        ret = sendPacket(packet);
    } while (0);
    return ret;
}

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_tools_live_ScreenLive_sendData(JNIEnv *env, jobject thiz, jbyteArray data_, jint len,
                                        jint type, jlong tms) {
    jbyte *data = env->GetByteArrayElements(data_, NULL);

    int ret;
    switch (type) {
        case 0: //video
            ret = sendVideo(data, len, tms);
            LOGI("send Video......");
            break;
        default: //audio
            ret = sendAudio(data, len, type, tms);
            LOGI("send Audio......");
            break;
    }
    env->ReleaseByteArrayElements(data_, data, 0);
    return ret;
}

JavaCallHelper *helper;
pthread_t pid;
char *path = nullptr;
uint64_t startTime = 0;
pthread_mutex_t mutex;
AudioChannel2 *audioChannel = nullptr;
VideoChannel2 *videoChannel = nullptr;

void callback(RTMPPacket *p) {
    if (rtmp) {
        p->m_nInfoField2 = rtmp->m_stream_id;
        p->m_nTimeStamp = RTMP_GetTime() - startTime;
        RTMP_SendPacket(rtmp, p, 1);
    }
    // TODO 为何Free后还需要delete
    RTMPPacket_Free(p);
    delete p;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_nativeInit(JNIEnv *env, jobject thiz) {
    helper = new JavaCallHelper(javaVM, env, thiz);
    pthread_mutex_init(&mutex, nullptr);
}

void *connect(void *) {
    int ret;
    do {
        rtmp = RTMP_Alloc();
        RTMP_Init(rtmp);
        if (!(ret = RTMP_SetupURL(rtmp, path))) break;
        // 开启输出模式
        RTMP_EnableWrite(rtmp);
        if (!(ret = RTMP_Connect(rtmp, 0))) break;
        if (!(ret = RTMP_ConnectStream(rtmp, 0))) break;
        RTMPPacket *packet = audioChannel->getAudioConfig();
        callback(packet);
    } while (0);

    if (!ret) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = nullptr;
    } else {
        // 通知Java可以开始推流
        helper->onParpare2(THREAD_CHILD, true);
        startTime = RTMP_GetTime();
    }

    delete path;
    path = nullptr;
    return nullptr;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_connect(JNIEnv *env, jobject thiz, jstring url_) {
    const char *url = env->GetStringUTFChars(url_, nullptr);
    path = new char[strlen(url) + 1];
    memcpy(path, url, strlen(url));
    pthread_create(&pid, nullptr, connect, (void *) url);
    env->ReleaseStringUTFChars(url_, url);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_initVideoEnc(JNIEnv *env, jobject thiz, jint width, jint height,
                                            jint fps, jint bit_rate) {
    // 准备好编码器
    videoChannel = new VideoChannel2;
    videoChannel->openCodec(width, height, fps, bit_rate);
    videoChannel->setCallback(callback);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_nativeSendVideo(JNIEnv *env, jobject thiz, jbyteArray buffer_) {
    jbyte *data = env->GetByteArrayElements(buffer_, nullptr);
    pthread_mutex_lock(&mutex);
    videoChannel->encode(reinterpret_cast<uint8_t *>(data));
    pthread_mutex_unlock(&mutex);
    env->ReleaseByteArrayElements(buffer_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_disConnect(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&mutex);
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = nullptr;
    }
    if (videoChannel) {
        videoChannel->resetPts();
    }
    pthread_mutex_unlock(&mutex);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_releaseVideoEnc(JNIEnv *env, jobject thiz) {
    if (videoChannel) {
        delete videoChannel;
        videoChannel = nullptr;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_nativeDeInit(JNIEnv *env, jobject thiz) {
    if (helper) {
        delete helper;
        helper = nullptr;
    }
    pthread_mutex_destroy(&mutex);
}

extern "C"

JNIEXPORT jint JNICALL
Java_com_tools_live_RtmpClient_initAudioEnc(JNIEnv *env, jobject thiz, jint sample_rate,
                                            jint channels) {
    audioChannel = new AudioChannel2();
    audioChannel->openCodec(sample_rate, channels);
    audioChannel->setCallback(callback);
    return audioChannel->getInputByteNum();
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_nativeSendAudio(JNIEnv *env, jobject thiz, jbyteArray buffer,
                                               jint len) {
    jbyte *data = env->GetByteArrayElements(buffer, nullptr);

    pthread_mutex_lock(&mutex);
    audioChannel->encode(reinterpret_cast<int32_t *>(data), len);
    pthread_mutex_unlock(&mutex);

    env->ReleaseByteArrayElements(buffer, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_releaseAudioEnc(JNIEnv *env, jobject thiz) {
    if (audioChannel) {
        delete audioChannel;
        audioChannel = nullptr;
    }
}

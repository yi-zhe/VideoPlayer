#include <jni.h>
#include <string>
#include "EnjoyPlayer.h"
#include "JavaCallHelper.h"

extern "C" {
#include <librtmp/rtmp.h>
#include "packt.h"
}
JavaVM *javaVM = 0;

JNIEXPORT jint

JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    return JNI_VERSION_1_4;
}


extern "C"
JNIEXPORT jlong

JNICALL
Java_com_tools_player_EnjoyPlayer_nativeInit(JNIEnv *env, jobject thiz) {
    EnjoyPlayer *player = new EnjoyPlayer(new JavaCallHelper(javaVM, env, thiz));
    return (jlong)
            player;
}


extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_setDataSource(JNIEnv
                                                *env,
                                                jobject thiz, jlong
                                                nativeHandle,
                                                jstring path_
) {
    const char *path = env->GetStringUTFChars(path_, 0);
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->
            setDataSource(path);

    env->
            ReleaseStringUTFChars(path_, path
    );
}



extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_prepare(JNIEnv
                                          *env,
                                          jobject thiz, jlong
                                          nativeHandle) {
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->

            prepare();

}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_start(JNIEnv
                                        *env,
                                        jobject thiz, jlong
                                        nativeHandle) {
    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    player->

            start();

}



extern "C"
JNIEXPORT void JNICALL
Java_com_tools_player_EnjoyPlayer_setSurface(JNIEnv
                                             *env,
                                             jobject thiz, jlong
                                             nativeHandle,
                                             jobject surface
) {

    EnjoyPlayer *player = reinterpret_cast<EnjoyPlayer *>(nativeHandle);
    ANativeWindow *window = ANativeWindow_fromSurface(env, surface);
    player->
            setWindow(window);
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
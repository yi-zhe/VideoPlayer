#include <jni.h>
#include <string>
#include "EnjoyPlayer.h"
#include "JavaCallHelper.h"

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

extern "C"
JNIEXPORT jboolean JNICALL
Java_com_tools_live_ScreenLive_connect(JNIEnv *env, jobject thiz, jstring url) {
    return true;
}
#include <jni.h>
#include <string>
#include <pthread.h>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc/types_c.h>
#include "EnjoyPlayer.h"
#include "JavaCallHelper.h"
#include "Log.h"
#include "VideoChannelWithX264.h"
#include "AudioChannelWithFaac.h"
#include "FaceTrack.h"

extern "C" {
#include <librtmp/rtmp.h>
#include "packt.h"
}

using namespace cv;

JavaVM *javaVM = 0;

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *reserved) {
    javaVM = vm;
    cv::DetectionBasedTracker::Parameters DetectorParams;
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
pthread_mutex_t myMutex;
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
    pthread_mutex_init(&myMutex, nullptr);
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
    pthread_mutex_lock(&myMutex);
    videoChannel->encode(reinterpret_cast<uint8_t *>(data));
    pthread_mutex_unlock(&myMutex);
    env->ReleaseByteArrayElements(buffer_, data, 0);
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_live_RtmpClient_disConnect(JNIEnv *env, jobject thiz) {
    pthread_mutex_lock(&myMutex);
    if (rtmp) {
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
        rtmp = nullptr;
    }
    if (videoChannel) {
        videoChannel->resetPts();
    }
    pthread_mutex_unlock(&myMutex);
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
    pthread_mutex_destroy(&myMutex);
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

    pthread_mutex_lock(&myMutex);
    audioChannel->encode(reinterpret_cast<int32_t *>(data), len);
    pthread_mutex_unlock(&myMutex);

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

//class CascadeDetectorAdapter : public DetectionBasedTracker::IDetector {
//public:
//    CascadeDetectorAdapter(Ptr<CascadeClassifier> detector) :
//            IDetector(), Detector(detector) {
//        CV_Assert(detector);
//    }
//
//    void detect(const Mat &Image, vector<Rect> &objects) {
//        Detector->detectMultiScale(Image, objects, scaleFactor, minNeighbours, 0, minObjSize,
//                                   maxObjSize);
//    }
//
//private:
//    CascadeDetectorAdapter();
//
//    Ptr<CascadeClassifier> Detector;
//};

class FaceTracker {
public:
    Ptr<DetectionBasedTracker> tracker;
    pthread_mutex_t detectMutex;
    ANativeWindow *window = nullptr;
public:
    FaceTracker(Ptr<DetectionBasedTracker> tracker) : tracker(tracker) {
        pthread_mutex_init(&detectMutex, nullptr);
    }

    ~FaceTracker() {
        pthread_mutex_destroy(&detectMutex);
        if (this->window) {
            ANativeWindow_release(this->window);
            this->window = nullptr;
        }
    }

    void setANativeWindow(ANativeWindow *w) {
        pthread_mutex_lock(&detectMutex);
        if (this->window) {
            ANativeWindow_release(this->window);
        }
        this->window = w;
        pthread_mutex_unlock(&detectMutex);
    }

    void draw(const Mat &img) {
        pthread_mutex_lock(&detectMutex);
        do {
            if (window) {
                ANativeWindow_setBuffersGeometry(window, img.cols, img.rows,
                                                 WINDOW_FORMAT_RGBA_8888);
                ANativeWindow_Buffer buffer;
                if (ANativeWindow_lock(window, &buffer, nullptr)) {
                    ANativeWindow_release(window);
                    break;
                }

                uint8_t *dstData = static_cast<uint8_t *>(buffer.bits);
                uint8_t *srcData = img.data;
                int dstLineSize = buffer.stride * 4;
                int srcLineSize = img.cols * 4;
                for (int i = 0; i < buffer.height; i++) {
                    memcpy(dstData + i * dstLineSize, srcData + i * img.cols * 4, srcLineSize);
                }
                ANativeWindow_unlockAndPost(window);
            }
        } while (false);
        pthread_mutex_unlock(&detectMutex);
    }
};

extern "C"
JNIEXPORT jlong JNICALL
Java_com_tools_cv_FaceTracker_nativeCreateObject(JNIEnv *env, jclass clazz, jstring model_) {
    const char *model = env->GetStringUTFChars(model_, 0);

    Ptr<CascadeDetectorAdapter> mainDetector = makePtr<CascadeDetectorAdapter>(
            makePtr<CascadeClassifier>(model));
    Ptr<CascadeDetectorAdapter> trackDetector = makePtr<CascadeDetectorAdapter>(
            makePtr<CascadeClassifier>(model));

    // 跟踪器
    DetectionBasedTracker::Parameters DetectorParams;
    FaceTracker *tracker = new FaceTracker(
            makePtr<DetectionBasedTracker>(
                    DetectionBasedTracker(mainDetector, trackDetector, DetectorParams)));

    env->ReleaseStringUTFChars(model_, model);
    return (jlong) tracker;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker_nativeDestroyObject(JNIEnv *env, jclass clazz, jlong thiz) {
    if (thiz != 0) {
        auto *tracker = (FaceTracker *) thiz;
        tracker->tracker->stop();
        delete tracker;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker_nativeSetSurface(JNIEnv *env, jclass clazz, jlong thiz,
                                               jobject surface) {
    if (thiz != 0) {
        auto *tracker = (FaceTracker *) thiz;
        if (!surface) {
            tracker->setANativeWindow(nullptr);
            return;
        }
        tracker->setANativeWindow(ANativeWindow_fromSurface(env, surface));
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker_nativeStart(JNIEnv *env, jclass clazz, jlong thiz) {
    if (thiz != 0) {
        auto *tracker = (FaceTracker *) thiz;
        tracker->tracker->run();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker_nativeStop(JNIEnv *env, jclass clazz, jlong thiz) {
    if (thiz != 0) {
        auto *tracker = (FaceTracker *) thiz;
        tracker->tracker->stop();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker_nativeDetect(JNIEnv *env, jclass clazz, jlong thiz,
                                           jbyteArray inputImage_, jint width, jint height,
                                           jint rotation_degrees) {
    if (thiz == 0) {
        return;
    }

    jbyte *inputImage = env->GetByteArrayElements(inputImage_, nullptr);

    // 8 8位 U unsigned C1 Channel通道 RGB是3通道
    Mat src(height * 3 / 2, width, CV_8UC1, inputImage);

    // 转为RGBA
    cvtColor(src, src, CV_YUV2RGBA_I420);
    // 旋转
    if (rotation_degrees == 90) {
        rotate(src, src, ROTATE_90_CLOCKWISE);
    } else if (rotation_degrees == 270) {
        rotate(src, src, ROTATE_90_COUNTERCLOCKWISE);
    }
//    flip(src, src, 1);
    Mat gray;
    cvtColor(src, gray, CV_RGB2GRAY);
    equalizeHist(gray, gray);

    auto *tracker = (FaceTracker *) thiz;
    tracker->tracker->process(gray);
    vector<cv::Rect> faces;
    tracker->tracker->getObjects(faces);
    for (cv::Rect face: faces) {
        // 花矩形
        rectangle(src, face, Scalar(255, 0, 255));
    }

    tracker->draw(src);


    env->ReleaseByteArrayElements(inputImage_, inputImage, 0);

}

extern "C"
JNIEXPORT jlong JNICALL
Java_com_tools_cv_FaceTracker2_nativeCreateObject(JNIEnv *env, jclass clazz, jstring facemodel_,
                                                  jstring landmarkermodel_) {
    const char *facemodel = env->GetStringUTFChars(facemodel_, 0);
    const char *landmarkermodel = env->GetStringUTFChars(landmarkermodel_, 0);

    FaceTrack *faceTrack = new FaceTrack(facemodel, landmarkermodel);

    env->ReleaseStringUTFChars(facemodel_, facemodel);
    env->ReleaseStringUTFChars(landmarkermodel_, landmarkermodel);
    return (jlong) faceTrack;
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker2_nativeDestroyObject(JNIEnv *env, jclass clazz, jlong thiz) {
    if (thiz != 0) {
        FaceTrack *tracker = reinterpret_cast<FaceTrack *>(thiz);
        tracker->stop();
        delete tracker;
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker2_nativeStart(JNIEnv *env, jclass clazz, jlong thiz) {
    if (thiz != 0) {
        FaceTrack *tracker = reinterpret_cast<FaceTrack *>(thiz);
        tracker->run();
    }
}

extern "C"
JNIEXPORT void JNICALL
Java_com_tools_cv_FaceTracker2_nativeStop(JNIEnv *env, jclass clazz, jlong thiz) {
    if (thiz != 0) {
        FaceTrack *tracker = reinterpret_cast<FaceTrack *>(thiz);
        tracker->stop();
    }
}

extern "C"
JNIEXPORT jobject JNICALL
Java_com_tools_cv_FaceTracker2_nativeDetect(JNIEnv *env, jclass clazz, jlong thiz,
                                            jbyteArray inputImage_, jint width, jint height,
                                            jint rotationDegrees) {
    if (thiz == 0) {
        return 0;
    }
    FaceTrack *tracker = reinterpret_cast<FaceTrack *>(thiz);
    jbyte *inputImage = env->GetByteArrayElements(inputImage_, 0);

    //I420
    Mat src(height * 3 / 2, width, CV_8UC1, inputImage);

    // 转为RGBA
    cvtColor(src, src, CV_YUV2RGBA_I420);
    //旋转
    if (rotationDegrees == 90) {
        rotate(src, src, ROTATE_90_CLOCKWISE);
    } else if (rotationDegrees == 270) {
        rotate(src, src, ROTATE_90_COUNTERCLOCKWISE);
    }
    //镜像问题，可以使用此方法进行垂直翻转
//    flip(src,src,1);
    Mat gray;
    cvtColor(src, gray, CV_RGBA2GRAY);
    equalizeHist(gray, gray);

    cv::Rect face;
    std::vector<SeetaPointF> points;
    tracker->process(gray, face, points);


    int w = src.cols;
    int h = src.rows;
    gray.release();
    src.release();
    env->ReleaseByteArrayElements(inputImage_, inputImage, 0);

    if (!face.empty() && !points.empty()) {
        jclass cls = env->FindClass("com/tools/cv/Face");
        jmethodID construct = env->GetMethodID(cls, "<init>", "(IIIIIIFFFF)V");
        SeetaPointF left = points[0];
        SeetaPointF right = points[1];
        jobject obj = env->NewObject(cls, construct, face.width, face.height, w, h, face.x, face.y,
                                     left.x, left.y, right.x, right.y);
        return obj;
    }

    return nullptr;
}
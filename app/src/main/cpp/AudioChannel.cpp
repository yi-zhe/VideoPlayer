//
// Created by liuyang on 2021/2/27.
//

#include "AudioChannel.h"

void *audioPlay_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->_play();
    return nullptr;
}


void *audioDecode_t(void *args) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(args);
    audioChannel->decode();
    return nullptr;
}

AudioChannel::AudioChannel(int channelId, JavaCallHelper *helper, AVCodecContext *avCodecContext,
                           const AVRational &base) : BaseChannel(channelId, helper, avCodecContext,
                                                                 base) {
    out_channels = av_get_channel_layout_nb_channels(AV_CH_LAYOUT_STEREO);
    //采樣位 2字節
    out_sampleSize = av_get_bytes_per_sample(AV_SAMPLE_FMT_S16);
    //采樣率
    int out_sampleRate = 44100;

    //計算轉換后數據的最大字節數
    bufferCount = out_sampleRate * out_sampleSize * out_channels;
    buffer = static_cast<uint8_t *>(malloc(bufferCount));
}

AudioChannel::~AudioChannel() {

}

void bqPlayerCallback(SLAndroidSimpleBufferQueueItf queue, void *pContext) {
    AudioChannel *audioChannel = static_cast<AudioChannel *>(pContext);
    int dataSize = audioChannel->_getData();
    if (dataSize > 0) {
        (*queue)->Enqueue(queue, audioChannel->buffer, dataSize);
    }
}

int AudioChannel::_getData() {
    int dataSize = 0;
    AVFrame *frame;
    while (isPlaying) {
        int ret = frame_queue.deQueue(frame);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }
        int nb = swr_convert(swrContext, &buffer, bufferCount,
                             (const uint8_t **) frame->data, frame->nb_samples);
        dataSize = nb * out_channels * out_sampleSize;
        break;
    }
    return dataSize;
}

void AudioChannel::_play() {
    /**
     * 1、创建引擎
     */
    // 创建引擎engineObject
    SLObjectItf engineObject = nullptr;
    SLresult result;
    result = slCreateEngine(&engineObject, 0, nullptr, 0, NULL, NULL);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }
    //初始化
    result = (*engineObject)->Realize(engineObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    // 获取引擎接口engineInterface
    SLEngineItf engineInterface = nullptr;
    result = (*engineObject)->GetInterface(engineObject, SL_IID_ENGINE,
                                           &engineInterface);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    /**
     * 2、创建混音器
     */
    //通过引擎接口创建混音器
    SLObjectItf outputMixObject = nullptr;
    result = (*engineInterface)->CreateOutputMix(engineInterface, &outputMixObject, 0, 0, 0);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }

    // 初始化混音器outputMixObject
    result = (*outputMixObject)->Realize(outputMixObject, SL_BOOLEAN_FALSE);
    if (SL_RESULT_SUCCESS != result) {
        return;
    }


    /**
     * 3、创建播放器
     */
    SLDataLocator_AndroidSimpleBufferQueue android_queue =
            {SL_DATALOCATOR_ANDROIDSIMPLEBUFFERQUEUE, 2};
    //pcm数据格式: pcm、声道数、采样率、采样位、容器大小、通道掩码(双声道)、字节序(小端)
    SLDataFormat_PCM pcm = {SL_DATAFORMAT_PCM, 2, SL_SAMPLINGRATE_44_1,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_PCMSAMPLEFORMAT_FIXED_16,
                            SL_SPEAKER_FRONT_LEFT | SL_SPEAKER_FRONT_RIGHT,
                            SL_BYTEORDER_LITTLEENDIAN};
    //数据源 （数据获取器+格式）  从哪里获得播放数据
    SLDataSource slDataSource = {&android_queue, &pcm};

    //设置混音器
    SLDataLocator_OutputMix outputMix = {SL_DATALOCATOR_OUTPUTMIX, outputMixObject};
    SLDataSink audioSnk = {&outputMix, nullptr};


    const SLInterfaceID ids[1] = {SL_IID_BUFFERQUEUE};
    const SLboolean req[1] = {SL_BOOLEAN_TRUE};

    //播放器相当于对混音器进行了一层包装， 提供了额外的如：开始，停止等方法。
    // 混音器来播放声音
    SLObjectItf bqPlayerObject = nullptr;
    (*engineInterface)->CreateAudioPlayer(engineInterface, &bqPlayerObject, &slDataSource,
                                          &audioSnk, 1, ids, req);
    //初始化播放器
    (*bqPlayerObject)->Realize(bqPlayerObject, SL_BOOLEAN_FALSE);

    //获得播放数据队列操作接口
    SLAndroidSimpleBufferQueueItf bqPlayerBufferQueue = nullptr;
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_BUFFERQUEUE, &bqPlayerBufferQueue);
    //设置回调（启动播放器后执行回调来获取数据并播放）
    (*bqPlayerBufferQueue)->RegisterCallback(bqPlayerBufferQueue, bqPlayerCallback, this);

    //获取播放状态接口
    SLPlayItf bqPlayerInterface = nullptr;
    (*bqPlayerObject)->GetInterface(bqPlayerObject, SL_IID_PLAY, &bqPlayerInterface);
    // 设置播放状态
    (*bqPlayerInterface)->SetPlayState(bqPlayerInterface, SL_PLAYSTATE_PLAYING);

    //还要手动调用一次 回调方法，才能开始播放
    bqPlayerCallback(bqPlayerBufferQueue, this);
}

void AudioChannel::play() {

    swrContext = swr_alloc_set_opts(
            nullptr, AV_CH_LAYOUT_STEREO,
            AV_SAMPLE_FMT_S16, 44100,
            avCodecContext->channel_layout,
            avCodecContext->sample_fmt,
            avCodecContext->sample_rate, 0,
            nullptr);

    swr_init(swrContext);

    setEnable(true);
    isPlaying = true;
    pthread_create(&audioDecodeTask, nullptr, audioDecode_t, this);
    pthread_create(&audioDecodeTask, nullptr, audioPlay_t, this);
}

void AudioChannel::stop() {

}

void AudioChannel::decode() {
    AVPacket *packet;
    while (isPlaying) {
        int ret = pkt_queue.deQueue(packet);
        if (!isPlaying) {
            break;
        }
        if (!ret) {
            continue;
        }

        ret = avcodec_send_packet(avCodecContext, packet);
        releaseAvPacket(packet);
        if (ret < 0) {
            break;
        }
        AVFrame *frame = av_frame_alloc();
        ret = avcodec_receive_frame(avCodecContext, frame);
        if (AVERROR(EAGAIN) == ret) {
            continue;
        } else if (ret < 0) {
            break;
        }

        frame_queue.enQueue(frame);
    }
}

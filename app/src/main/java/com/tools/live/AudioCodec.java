package com.tools.live;

import android.media.AudioFormat;
import android.media.AudioRecord;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.MediaRecorder;
import java.io.IOException;
import java.nio.ByteBuffer;

public class AudioCodec extends Thread {

  MediaCodec mediaCodec;
  AudioRecord audioRecord;
  boolean isRecording;
  int bufSize;
  int sampleRateInHz = 44100;
  int channelInMono = AudioFormat.CHANNEL_IN_MONO;
  int encodingPcm16bit = AudioFormat.ENCODING_PCM_16BIT;
  long startTime;
  ScreenLive screenLive;

  public AudioCodec(ScreenLive s) {
    screenLive = s;
  }

  public void startLive() {
    // 1. 准备编码器
    try {
      mediaCodec = MediaCodec.createByCodecName(MediaFormat.MIMETYPE_AUDIO_AAC);
      MediaFormat mediaFormat =
          MediaFormat.createAudioFormat(MediaFormat.MIMETYPE_AUDIO_AAC, sampleRateInHz, 1);
      mediaFormat.setInteger(MediaFormat.KEY_AAC_PROFILE,
          MediaCodecInfo.CodecProfileLevel.AACObjectLC);
      mediaFormat.setInteger(MediaFormat.KEY_BIT_RATE, 64_000);
      mediaCodec.configure(mediaFormat, null, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
    } catch (IOException e) {
      e.printStackTrace();
    }

    bufSize = AudioRecord.getMinBufferSize(sampleRateInHz, channelInMono, encodingPcm16bit);
    audioRecord = new AudioRecord(MediaRecorder.AudioSource.MIC, sampleRateInHz, channelInMono,
        encodingPcm16bit, bufSize);

    start();
  }

  @Override
  public void run() {
    isRecording = true;
    mediaCodec.start();

    // 在获取播放的音频数据前 先发送audio specific config
    RTMPPackage rtmpPackage = new RTMPPackage();
    rtmpPackage.setBuffer(new byte[] { 0x12, 0x08 });
    rtmpPackage.setType(RTMPPackage.RTMP_PACKET_TYPE_AUDIO_HEAD);
    rtmpPackage.setTms(0);

    byte[] buffer = new byte[bufSize];
    MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
    while (isRecording) {
      int read = audioRecord.read(buffer, 0, buffer.length);
      if (read <= 0) {
        continue;
      }

      int inputIndex = mediaCodec.dequeueInputBuffer(10);
      if (inputIndex >= 0) {
        ByteBuffer byteBuffer = mediaCodec.getInputBuffer(inputIndex);
        byteBuffer.clear();
        byteBuffer.put(buffer, 0, read);
        // 通知容器我们用完了, 可以拿去编码
        mediaCodec.queueInputBuffer(inputIndex, 0, read,
            System.nanoTime() / 1000, 0);
      }

      int outputIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10);
      while (outputIndex >= 0 && isRecording) {
        ByteBuffer outByteBuffer = mediaCodec.getOutputBuffer(outputIndex);
        byte[] data = new byte[bufferInfo.size];
        outByteBuffer.get(data);
        // data 就是编码后的音频数据

        rtmpPackage = new RTMPPackage();
        rtmpPackage.setBuffer(data);
        rtmpPackage.setType(RTMPPackage.RTMP_PACKET_TYPE_AUDIO_DATA);
        if (startTime == 0) {
          startTime = bufferInfo.presentationTimeUs / 1000;
        }
        // 相对时间
        rtmpPackage.setTms(bufferInfo.presentationTimeUs / 1000 - startTime);
        screenLive.addPacket(rtmpPackage);

        // 释放输出队列
        mediaCodec.releaseOutputBuffer(outputIndex, false);
        outputIndex = mediaCodec.dequeueOutputBuffer(bufferInfo, 10);
      }
    }
    isRecording = false;
    audioRecord.stop();
    mediaCodec.stop();
    mediaCodec.release();
  }
}

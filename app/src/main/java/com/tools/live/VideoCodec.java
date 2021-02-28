package com.tools.live;

import android.hardware.display.DisplayManager;
import android.hardware.display.VirtualDisplay;
import android.media.MediaCodec;
import android.media.MediaCodecInfo;
import android.media.MediaFormat;
import android.media.projection.MediaProjection;
import android.os.Bundle;
import android.view.Surface;
import java.io.IOException;
import java.nio.ByteBuffer;

public class VideoCodec extends Thread {

  private Surface surface;
  private MediaCodec mediaCodec;
  private boolean isLiving;
  private long timestamp;
  private MediaProjection mediaProjection;
  private VirtualDisplay virtualDisplay;

  public void startLive(MediaProjection mediaProjection) {
    this.mediaProjection = mediaProjection;
    MediaFormat videoFormat =
        MediaFormat.createVideoFormat(MediaFormat.MIMETYPE_VIDEO_AVC, 360, 640);
    videoFormat.setInteger(MediaFormat.KEY_BIT_RATE, 400_000);
    videoFormat.setInteger(MediaFormat.KEY_FRAME_RATE, 15);
    // 关键帧间隔2s
    videoFormat.setInteger(MediaFormat.KEY_I_FRAME_INTERVAL, 2);

    videoFormat.setInteger(MediaFormat.KEY_COLOR_FORMAT,
        MediaCodecInfo.CodecCapabilities.COLOR_FormatSurface);

    try {
      mediaCodec = MediaCodec.createEncoderByType(MediaFormat.MIMETYPE_VIDEO_AVC);
      mediaCodec.configure(videoFormat, surface, null, MediaCodec.CONFIGURE_FLAG_ENCODE);
      surface = mediaCodec.createInputSurface();
      virtualDisplay = mediaProjection.createVirtualDisplay("screen",
          360, 640, 1,
          DisplayManager.VIRTUAL_DISPLAY_FLAG_PUBLIC,
          surface, null, null);
    } catch (IOException e) {
      e.printStackTrace();
    }
    isLiving = true;
    start();
  }

  public void stopLive() {
    isLiving = false;
  }

  @Override
  public void run() {
    super.run();
    mediaCodec.start();
    MediaCodec.BufferInfo bufferInfo = new MediaCodec.BufferInfo();
    while (isLiving) {
      // TODO MediaCodec关键帧问题
      // 手动刷新关键帧
      if (timestamp != 0) {
        if (System.currentTimeMillis() - timestamp >= 2) {
          // 刷新关键帧
          Bundle bundle = new Bundle();
          bundle.putInt(MediaCodec.PARAMETER_KEY_REQUEST_SYNC_FRAME, 0);
          mediaCodec.setParameters(bundle);
          timestamp = System.currentTimeMillis();
        }
      } else {
        timestamp = System.currentTimeMillis();
      }

      // 输出队列的下标
      int index = mediaCodec.dequeueOutputBuffer(bufferInfo, 10);
      if (index >= 0) {
        // 成功拿到数据
        ByteBuffer buffer = mediaCodec.getOutputBuffer(index);
        byte[] data = new byte[bufferInfo.size];
        buffer.get(data);

        // 送到C++中封包再发送

        // 释放
        mediaCodec.releaseOutputBuffer(index, false);
      }
    }
    isLiving = false;
    mediaCodec.stop();
    mediaCodec.release();
    virtualDisplay.release();
    mediaProjection.stop();
  }
}

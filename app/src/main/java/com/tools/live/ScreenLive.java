package com.tools.live;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import androidx.annotation.Nullable;
import java.util.concurrent.LinkedBlockingQueue;

public class ScreenLive implements Runnable {

  public static final int ScreenCaptureCode = 100;
  private String url;
  private MediaProjectionManager manager;
  private MediaProjection mediaProjection;
  private VideoCodec videoCodec;
  private AudioCodec audioCodec;
  private LinkedBlockingQueue<RTMPPackage> queue = new LinkedBlockingQueue<>();
  private boolean isLiving;

  public void startLive(String url, Activity cxt) {
    this.url = url;
    manager =
        (MediaProjectionManager) cxt.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
    Intent screenCaptureIntent = manager.createScreenCaptureIntent();
    cxt.startActivityForResult(screenCaptureIntent, ScreenCaptureCode);
  }

  public void stopLive() {
    isLiving = false;
    videoCodec.stopLive();
  }

  public void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
    if (requestCode == ScreenCaptureCode && resultCode == Activity.RESULT_OK) {
      // Surface 离屏画布
      // 怎样从Surface中获得图像数据
      mediaProjection = manager.getMediaProjection(resultCode, data);
      LiveExecutors.getInstance().execute(this);
    }
  }

  public void addPacket(RTMPPackage p) {
    queue.offer(p);
  }

  @Override
  public void run() {
    if (!connect(url)) {
      return;
    }
    videoCodec = new VideoCodec(this);
    videoCodec.startLive(mediaProjection);

    audioCodec = new AudioCodec(this);
    //audioCodec.startLive();
    isLiving = true;
    // 发送数据包
    while (isLiving) {
      RTMPPackage p = null;
      try {
        p = queue.take();
      } catch (InterruptedException e) {
        e.printStackTrace();
      }
      if (p == null) {
        break;
      }
      byte[] buffer = p.getBuffer();
      if (buffer != null) {
        sendData(buffer, buffer.length, p.getType(), p.getTms());
      }
    }
  }

  private native boolean connect(String url);

  private native boolean sendData(byte[] data, int len, int type, long tms);
}

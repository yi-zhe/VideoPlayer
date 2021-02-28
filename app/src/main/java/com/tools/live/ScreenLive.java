package com.tools.live;

import android.app.Activity;
import android.content.Context;
import android.content.Intent;
import android.media.projection.MediaProjection;
import android.media.projection.MediaProjectionManager;
import androidx.annotation.Nullable;

public class ScreenLive implements Runnable {

  public static final int ScreenCaptureCode = 100;
  private String url;
  private MediaProjectionManager manager;
  private MediaProjection mediaProjection;
  private VideoCodec videoCodec;

  public void startLive(String url, Activity cxt) {
    this.url = url;
    manager =
        (MediaProjectionManager) cxt.getSystemService(Context.MEDIA_PROJECTION_SERVICE);
    Intent screenCaptureIntent = manager.createScreenCaptureIntent();
    cxt.startActivityForResult(screenCaptureIntent, ScreenCaptureCode);
  }

  public void stopLive() {
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

  @Override
  public void run() {
    if (!connect(url)) {
      return;
    }
    videoCodec = new VideoCodec();
    videoCodec.startLive(mediaProjection);
  }

  private native boolean connect(String url);
}

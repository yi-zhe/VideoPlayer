package com.tools.live;

import android.util.Log;
import android.view.TextureView;
import androidx.lifecycle.LifecycleOwner;

public class RtmpClient {

  private static final String TAG = "RtmpClient";
  private final LifecycleOwner lifecycleOwner;

  private int width;
  private int height;
  private volatile boolean isConnected;
  private VideoChannel videoChannel;

  public RtmpClient(LifecycleOwner lifecycleOwner) {
    this.lifecycleOwner = lifecycleOwner;
    nativeInit();
  }

  public void initVideo(TextureView display, int width, int height, int fps, int bitRate) {
    this.width = width;
    this.height = height;
    videoChannel = new VideoChannel(lifecycleOwner, display, this);
    initVideoEnc(width, height, fps, bitRate);
  }

  public void toggleCamera() {
    videoChannel.toggleCamera();
  }

  public int getWidth() {
    return width;
  }

  public int getHeight() {
    return height;
  }

  public boolean isConnected() {
    return isConnected;
  }

  public void startLive(String url) {
    connect(url);
  }

  public void stopLive() {
    isConnected = false;
    disConnect();
    Log.e(TAG, "停止直播==================");
  }

  /**
   * JNICall
   */
  private void onPrepare(boolean isConnect) {
    // 是否有多线程问题 子线程后值更改 对其他线程可见吗
    this.isConnected = isConnect;
    Log.e(TAG, "开始直播==================");
    //通知UI
  }

  public void sendVideo(byte[] buffer) {
    nativeSendVideo(buffer);
  }

  public void release() {
    videoChannel.release();
    releaseVideoEnc();
    nativeDeInit();
  }

  private native void connect(String url);

  private native void disConnect();

  private native void nativeInit();

  private native void initVideoEnc(int width, int height, int fps, int bitRate);

  private native void releaseVideoEnc();

  private native void nativeDeInit();

  private native void nativeSendVideo(byte[] buffer);
}

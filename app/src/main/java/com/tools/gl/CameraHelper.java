package com.tools.gl;

import android.os.Handler;
import android.os.HandlerThread;
import android.util.Size;
import androidx.camera.core.CameraX;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageAnalysisConfig;
import androidx.camera.core.ImageProxy;
import androidx.camera.core.Preview;
import androidx.camera.core.PreviewConfig;
import androidx.lifecycle.Lifecycle;
import androidx.lifecycle.LifecycleObserver;
import androidx.lifecycle.LifecycleOwner;
import androidx.lifecycle.OnLifecycleEvent;
import com.tools.cv.Face;
import com.tools.cv.FaceTracker2;
import com.tools.live.ImageUtils;

public class CameraHelper implements ImageAnalysis.Analyzer, LifecycleObserver {

  private FaceTracker2 faceTracker;
  private HandlerThread handlerThread;
  private CameraX.LensFacing currentFacing = CameraX.LensFacing.FRONT;
  private Preview.OnPreviewOutputUpdateListener listener;
  private Face face;

  public CameraHelper(LifecycleOwner lifecycleOwner,
      Preview.OnPreviewOutputUpdateListener listener) {
    this.listener = listener;
    lifecycleOwner.getLifecycle().addObserver(this);
    handlerThread = new HandlerThread("Analyze-thread");
    handlerThread.start();
    CameraX.bindToLifecycle(lifecycleOwner, getPreView(), getImageAnalysis());
  }

  private Preview getPreView() {
    // 分辨率并不是最终的分辨率，CameraX会自动根据设备的支持情况，结合你的参数，设置一个最为接近的分辨率
    PreviewConfig previewConfig = new PreviewConfig.Builder()
        .setTargetResolution(new Size(640, 480))
        .setLensFacing(currentFacing) //前置或者后置摄像头
        .build();
    Preview preview = new Preview(previewConfig);
    preview.setOnPreviewOutputUpdateListener(listener);
    return preview;
  }

  private ImageAnalysis getImageAnalysis() {
    ImageAnalysisConfig imageAnalysisConfig = new ImageAnalysisConfig.Builder()
        .setCallbackHandler(new Handler(handlerThread.getLooper()))
        .setLensFacing(currentFacing).setTargetResolution(new Size(640, 480))
        .setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
        .build();
    ImageAnalysis imageAnalysis = new ImageAnalysis(imageAnalysisConfig);
    imageAnalysis.setAnalyzer(this);
    return imageAnalysis;
  }

  @Override
  public void analyze(ImageProxy image, int rotationDegrees) {
    byte[] bytes = ImageUtils.getBytes(image);
    synchronized (this) {
      face = faceTracker.detect(bytes, image.getWidth(), image.getHeight(), rotationDegrees);
    }
  }

  public synchronized Face getFace() {
    return face;
  }

  @OnLifecycleEvent(Lifecycle.Event.ON_CREATE)
  public void onCreate(LifecycleOwner owner) {
    faceTracker = new FaceTracker2("/sdcard/lbpcascade_frontalface.xml",
        "/sdcard/pd_2_00_pts5.dat");
  }

  @OnLifecycleEvent(Lifecycle.Event.ON_START)
  public void onStart(LifecycleOwner owner) {
    faceTracker.start();
  }

  @OnLifecycleEvent(Lifecycle.Event.ON_STOP)
  public void onStop(LifecycleOwner owner) {
    faceTracker.stop();
  }

  @OnLifecycleEvent(Lifecycle.Event.ON_DESTROY)
  public void onDestroy(LifecycleOwner owner) {
    faceTracker.release();
    owner.getLifecycle().removeObserver(this);
  }
}

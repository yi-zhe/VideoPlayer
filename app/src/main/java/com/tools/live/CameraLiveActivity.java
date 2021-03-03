package com.tools.live;

import android.graphics.SurfaceTexture;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Size;
import android.view.TextureView;
import android.view.View;
import android.view.ViewGroup;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.camera.core.CameraX;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageAnalysisConfig;
import androidx.camera.core.ImageProxy;
import androidx.camera.core.Preview;
import androidx.camera.core.PreviewConfig;
import com.tools.player.R;

public class CameraLiveActivity extends AppCompatActivity
    implements ImageAnalysis.Analyzer, Preview.OnPreviewOutputUpdateListener {
  private TextureView textureView;
  private HandlerThread handlerThread;
  private CameraX.LensFacing currentFacing = CameraX.LensFacing.BACK;
  private RtmpClient rtmpClient;

  private static final int width = 360;
  private static final int height = 640;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_camera_live);
    textureView = findViewById(R.id.textureView);
    //子线程中回调
    handlerThread = new HandlerThread("Analyze-thread");
    handlerThread.start();
    rtmpClient = new RtmpClient(width, height, 10, 640_000);
    CameraX.bindToLifecycle(this, getPreView(), getAnalysis());
  }

  public void startLive(View view) {
  }

  public void stopLive(View view) {
  }

  private ImageAnalysis getAnalysis() {
    ImageAnalysisConfig imageAnalysisConfig = new ImageAnalysisConfig.Builder()
        .setCallbackHandler(new Handler(handlerThread.getLooper()))
        .setLensFacing(currentFacing)
        .setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
        .setTargetResolution(new Size(width, height))
        .build();
    ImageAnalysis imageAnalysis = new ImageAnalysis(imageAnalysisConfig);
    imageAnalysis.setAnalyzer(this);
    return imageAnalysis;
  }

  private Preview getPreView() {
    // 分辨率并不是最终的分辨率，CameraX会自动根据设备的支持情况，结合你的参数，设置一个最为接近的分辨率
    PreviewConfig previewConfig = new PreviewConfig.Builder()
        .setTargetResolution(new Size(width, height))
        .setLensFacing(currentFacing) //前置或者后置摄像头
        .build();
    Preview preview = new Preview(previewConfig);
    preview.setOnPreviewOutputUpdateListener(this);
    return preview;
  }

  public void toggleCamera(View view) {
    CameraX.unbindAll();
    if (currentFacing == CameraX.LensFacing.BACK) {
      currentFacing = CameraX.LensFacing.FRONT;
    } else {
      currentFacing = CameraX.LensFacing.BACK;
    }
    CameraX.bindToLifecycle(this, getPreView(), getAnalysis());
  }

  @Override
  public void analyze(ImageProxy image, int rotationDegrees) {
    byte[] bytes =
        ImageUtils.getBytes(image, rotationDegrees, rtmpClient.getWidth(), rtmpClient.getHeight());
  }

  @Override
  public void onUpdated(Preview.PreviewOutput output) {
    SurfaceTexture surfaceTexture = output.getSurfaceTexture();
    if (textureView.getSurfaceTexture() != surfaceTexture) {
      if (textureView.isAvailable()) {
        // 当切换摄像头时，会报错
        ViewGroup parent = (ViewGroup) textureView.getParent();
        parent.removeView(textureView);
        parent.addView(textureView, 0);
        parent.requestLayout();
      }
      textureView.setSurfaceTexture(surfaceTexture);
    }
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    handlerThread.quitSafely();
  }
}

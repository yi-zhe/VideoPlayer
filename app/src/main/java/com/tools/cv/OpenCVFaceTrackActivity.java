package com.tools.cv;

import android.Manifest;
import android.os.Bundle;
import android.os.Handler;
import android.os.HandlerThread;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import android.view.View;
import androidx.annotation.NonNull;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import androidx.camera.core.CameraX;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageAnalysisConfig;
import androidx.camera.core.ImageProxy;
import androidx.camera.core.UseCase;
import com.permissionx.guolindev.PermissionX;
import com.permissionx.guolindev.callback.RequestCallback;
import com.tools.live.ImageUtils;
import com.tools.player.R;
import java.util.List;

public class OpenCVFaceTrackActivity extends AppCompatActivity implements ImageAnalysis.Analyzer {

  FaceTracker faceTracker;
  HandlerThread handlerThread;
  Handler handler;
  private CameraX.LensFacing currentFacing = CameraX.LensFacing.BACK;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_face_track);
    requestPermission(this);
    String path = Utils.copyAsset2Sdcard(this, "lbpcascade_frontalface.xml");
    faceTracker = new FaceTracker(path);
    faceTracker.start();

    SurfaceView surfaceView = findViewById(R.id.surfaceView);
    surfaceView.getHolder().addCallback(new SurfaceHolder.Callback() {
      @Override
      public void surfaceCreated(@NonNull SurfaceHolder holder) {

      }

      @Override
      public void surfaceChanged(@NonNull SurfaceHolder holder, int format, int width, int height) {
        faceTracker.setSurface(holder.getSurface());
      }

      @Override
      public void surfaceDestroyed(@NonNull SurfaceHolder holder) {
        faceTracker.setSurface(null);
      }
    });

    handlerThread = new HandlerThread("Analyze");
    handlerThread.start();
    handler = new Handler(handlerThread.getLooper());
    CameraX.bindToLifecycle(this, getAnalysis());
  }

  private UseCase getAnalysis() {
    ImageAnalysisConfig config = new ImageAnalysisConfig.Builder()
        .setCallbackHandler(handler)
        .setLensFacing(currentFacing)
        .setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
        .build();
    ImageAnalysis imageAnalysis = new ImageAnalysis(config);
    imageAnalysis.setAnalyzer(this);
    return imageAnalysis;
  }

  public static void requestPermission(AppCompatActivity act) {
    PermissionX.init(act)
        .permissions(Manifest.permission.RECORD_AUDIO, Manifest.permission.CAMERA)
        .request(new RequestCallback() {
          @Override
          public void onResult(boolean allGranted, List<String> grantedList,
              List<String> deniedList) {

          }
        });
  }

  public void toggleCamera(View view) {
    CameraX.unbindAll();
    if (currentFacing == CameraX.LensFacing.BACK) {
      currentFacing = CameraX.LensFacing.FRONT;
    } else {
      currentFacing = CameraX.LensFacing.BACK;
    }
    CameraX.bindToLifecycle(this, getAnalysis());
  }

  @Override
  public void analyze(ImageProxy image, int rotationDegrees) {
    byte[] bytes = ImageUtils.getBytes(image);
    faceTracker.detect(bytes, image.getWidth(), image.getHeight(), rotationDegrees);
  }
}

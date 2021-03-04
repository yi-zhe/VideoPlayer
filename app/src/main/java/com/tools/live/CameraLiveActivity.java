package com.tools.live;

import android.os.Bundle;
import android.view.TextureView;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import com.tools.player.R;

public class CameraLiveActivity extends AppCompatActivity {
  private RtmpClient rtmpClient;

  private static final int width = 360;
  private static final int height = 640;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_camera_live);
    TextureView textureView = findViewById(R.id.textureView);
    rtmpClient = new RtmpClient(this);
    //初始化摄像头， 同时 创建编码器
    rtmpClient.initVideo(textureView, width, height, 10, 640_000);
  }

  public void startLive(View view) {
    rtmpClient.startLive("rtmp://10.36.216.39:1935/live");
  }

  public void stopLive(View view) {
    rtmpClient.stopLive();
  }

  public void toggleCamera(View view) {
    rtmpClient.toggleCamera();
  }

  @Override
  protected void onDestroy() {
    super.onDestroy();
    rtmpClient.release();
  }
}

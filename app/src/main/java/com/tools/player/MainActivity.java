package com.tools.player;

import android.os.Bundle;
import android.view.Surface;
import android.view.SurfaceHolder;
import android.view.SurfaceView;
import androidx.appcompat.app.AppCompatActivity;

public class MainActivity extends AppCompatActivity implements SurfaceHolder.Callback {

  EnjoyPlayer player;

  @Override
  protected void onCreate(Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_main);
    SurfaceView surfaceView = findViewById(R.id.surfaceView);
    surfaceView.getHolder().addCallback(this);

    player = new EnjoyPlayer();
    player.setDataSource("/sdcard/input.mp4");
    player.setOnPrepareListener(new EnjoyPlayer.OnPrepareListener() {
      @Override
      public void onPrepared() {
        player.start();
      }
    });
    player.prepare();
  }

  @Override
  public void surfaceCreated(SurfaceHolder holder) {

  }

  @Override
  public void surfaceChanged(SurfaceHolder holder, int format, int width, int height) {
    Surface surface = holder.getSurface();
    player.setSurface(surface);
  }

  @Override
  public void surfaceDestroyed(SurfaceHolder holder) {

  }

  @Override
  protected void onStop() {
    super.onStop();
    player.stop();
  }
}

package com.tools.live;

import android.content.Intent;
import android.os.Bundle;
import android.view.View;
import androidx.annotation.Nullable;
import androidx.appcompat.app.AppCompatActivity;
import com.tools.player.R;

public class ScreenLiveActivity extends AppCompatActivity {

  ScreenLive screenLive;

  @Override
  protected void onCreate(@Nullable Bundle savedInstanceState) {
    super.onCreate(savedInstanceState);
    setContentView(R.layout.activity_live);
    screenLive = new ScreenLive();
  }

  @Override
  protected void onActivityResult(int requestCode, int resultCode, @Nullable Intent data) {
    super.onActivityResult(requestCode, resultCode, data);
    screenLive.onActivityResult(requestCode, resultCode, data);
  }

  public void start(View view) {
    screenLive.startLive("rtmp://10.36.216.39:1935/live", this);
  }

  public void stop(View view) {
    screenLive.stopLive();
  }
}

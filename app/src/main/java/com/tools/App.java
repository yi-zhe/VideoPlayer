package com.tools;

import android.app.Application;

public class App extends Application {

  static {
    System.loadLibrary("native-lib");
  }
}

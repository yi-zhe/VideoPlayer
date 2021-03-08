package com.tools.gl;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.GLSurfaceView;
import androidx.camera.core.Preview;
import androidx.lifecycle.LifecycleOwner;
import com.tools.gl.filter.CameraFilter;
import com.tools.gl.filter.ScreenFilter;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class CameraRenderer
    implements GLSurfaceView.Renderer, Preview.OnPreviewOutputUpdateListener,
    SurfaceTexture.OnFrameAvailableListener {
  private CameraView cameraView;
  private CameraHelper cameraHelper;
  // 摄像头的图像  用OpenGL ES 画出来
  private SurfaceTexture mCameraTexure;

  private int[] textures;
  private ScreenFilter screenFilter;

  float[] mtx = new float[16];
  CameraFilter cameraFilter;

  public CameraRenderer(CameraView cameraView) {
    this.cameraView = cameraView;
    LifecycleOwner lifecycleOwner = (LifecycleOwner) cameraView.getContext();
    cameraHelper = new CameraHelper(lifecycleOwner, this);
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    // 创建OpenGL纹理 把摄像头的数据与纹理关联
    textures = new int[1]; // 能在OpenGL中用的图片的id
    mCameraTexure.attachToGLContext(textures[0]);
    // 摄像头数据有更新的时候会回调
    mCameraTexure.setOnFrameAvailableListener(this);

    Context cxt = cameraView.getContext();
    cameraFilter = new CameraFilter(cxt);

    screenFilter = new ScreenFilter(cameraView.getContext());
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    cameraFilter.setSize(width, height);
    screenFilter.setSize(width, height);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    // 更新纹理
    mCameraTexure.updateTexImage();
    mCameraTexure.getTransformMatrix(mtx);

    cameraFilter.setTransformMatrix(mtx);
    int id = cameraFilter.onDraw(textures[0]);
    screenFilter.onDraw(id);
  }

  public void onSurfaceDestroyed() {
    cameraFilter.release();
    screenFilter.release();
  }

  /**
   * 更新
   *
   * @param output 预览输出
   */
  @Override
  public void onUpdated(Preview.PreviewOutput output) {
    mCameraTexure = output.getSurfaceTexture();
  }

  @Override
  public void onFrameAvailable(SurfaceTexture surfaceTexture) {
    cameraView.requestRender();
  }
}

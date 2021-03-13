package com.tools.gl;

import android.content.Context;
import android.graphics.SurfaceTexture;
import android.opengl.EGL14;
import android.opengl.GLSurfaceView;
import androidx.camera.core.Preview;
import androidx.lifecycle.LifecycleOwner;
import com.tools.gl.filter.AbstractFilter;
import com.tools.gl.filter.CameraFilter;
import com.tools.gl.filter.FilterChain;
import com.tools.gl.filter.FilterContext;
import com.tools.gl.filter.ScreenFilter;
import com.tools.gl.filter.SoulFilter;
import com.tools.gl.filter.SplitFilter;
import com.tools.gl.filter.StickFilter;
import com.tools.gl.filter.beauty.BeautyFilter;
import com.tools.gl.record.MediaRecorder;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.opengles.GL10;

public class CameraRenderer
    implements GLSurfaceView.Renderer, Preview.OnPreviewOutputUpdateListener,
    SurfaceTexture.OnFrameAvailableListener {
  private CameraView cameraView;
  private CameraHelper cameraHelper;
  // 摄像头的图像  用OpenGL ES 画出来
  private SurfaceTexture mCameraTexture;

  private int[] textures;

  float[] mtx = new float[16];
  private MediaRecorder mRecorder;
  private FilterChain filterChain;

  public CameraRenderer(CameraView cameraView) {
    this.cameraView = cameraView;
    OpenGLUtils.copyAssets2SdCard(cameraView.getContext(), "lbpcascade_frontalface.xml",
        "/sdcard/lbpcascade_frontalface.xml");
    OpenGLUtils.copyAssets2SdCard(cameraView.getContext(), "pd_2_00_pts5.dat",
        "/sdcard/pd_2_00_pts5.dat");
    LifecycleOwner lifecycleOwner = (LifecycleOwner) cameraView.getContext();
    cameraHelper = new CameraHelper(lifecycleOwner, this);
  }

  @Override
  public void onSurfaceCreated(GL10 gl, EGLConfig config) {
    // 创建OpenGL纹理 把摄像头的数据与纹理关联
    textures = new int[1]; // 能在OpenGL中用的图片的id
    mCameraTexture.attachToGLContext(textures[0]);
    // 摄像头数据有更新的时候会回调
    mCameraTexture.setOnFrameAvailableListener(this);

    Context cxt = cameraView.getContext();
    List<AbstractFilter> filters = new ArrayList<>();
    filters.add(new CameraFilter(cxt));
    filters.add(new StickFilter(cxt));
    filters.add(new SplitFilter(cxt));
    filters.add(new ScreenFilter(cxt));

    filterChain = new FilterChain(filters, 0, new FilterContext());

    mRecorder = new MediaRecorder(cameraView.getContext(), "/sdcard/a.mp4",
        EGL14.eglGetCurrentContext(),
        480, 640);
  }

  @Override
  public void onSurfaceChanged(GL10 gl, int width, int height) {
    filterChain.setSize(width, height);
  }

  @Override
  public void onDrawFrame(GL10 gl) {
    // 更新纹理
    mCameraTexture.updateTexImage();
    mCameraTexture.getTransformMatrix(mtx);

    filterChain.setTransformMatrix(mtx);
    filterChain.setFace(cameraHelper.getFace());
    int id = filterChain.proceed(textures[0]);
    mRecorder.fireFrame(id, mCameraTexture.getTimestamp());
  }

  public void onSurfaceDestroyed() {
    filterChain.release();
  }

  /**
   * 更新
   *
   * @param output 预览输出
   */
  @Override
  public void onUpdated(Preview.PreviewOutput output) {
    mCameraTexture = output.getSurfaceTexture();
  }

  @Override
  public void onFrameAvailable(SurfaceTexture surfaceTexture) {
    cameraView.requestRender();
  }

  public void startRecord(float speed) {
    try {
      mRecorder.start(speed);
    } catch (IOException e) {
      e.printStackTrace();
    }
  }

  public void stopRecord() {
    mRecorder.stop();
  }
}

package com.tools.gl.record;

import android.content.Context;
import android.opengl.EGL14;
import android.opengl.EGLConfig;
import android.opengl.EGLContext;
import android.opengl.EGLDisplay;
import android.opengl.EGLExt;
import android.opengl.EGLSurface;
import android.view.Surface;
import com.tools.gl.filter.FilterChain;
import com.tools.gl.filter.FilterContext;
import com.tools.gl.filter.RecordFilter;
import java.util.ArrayList;

public class EGLEnv {

  private final EGLDisplay eglDisplay;
  private EGLContext mGlContext;
  EGLConfig mEglConfig;
  EGLContext mEglContext;
  EGLSurface mEglSurface;
  Surface surface;
  RecordFilter recordFilter;
  private FilterChain filterChain;

  public EGLEnv(Context context, EGLContext cxt, Surface surface, int width, int height) {
    // 创建显示窗口 作为OpenGL的绘制目标
    eglDisplay = EGL14.eglGetDisplay(EGL14.EGL_DEFAULT_DISPLAY);
    this.surface = surface;
    this.mGlContext = cxt;
    if (EGL14.EGL_NO_DISPLAY == eglDisplay) {
      throw new RuntimeException("eglInitialize failed");
    }

    // 初始化显示窗口
    int[] version = new int[2];
    if (!EGL14.eglInitialize(eglDisplay, version, 0, version, 1)) {
      throw new RuntimeException("eglInitialize failed");
    }

    // 配置 属性选项
    int[] configAttribs = {
        EGL14.EGL_RED_SIZE, 8, //颜色缓冲区中红色位数
        EGL14.EGL_GREEN_SIZE, 8,//颜色缓冲区中绿色位数
        EGL14.EGL_BLUE_SIZE, 8, //
        EGL14.EGL_ALPHA_SIZE, 8,//
        EGL14.EGL_RENDERABLE_TYPE,
        EGL14.EGL_OPENGL_ES2_BIT, //opengl es 2.0
        EGL14.EGL_NONE
    };
    int[] numConfigs = new int[1];
    EGLConfig[] configs = new EGLConfig[1];
    //EGL 根据属性选择一个配置
    if (!EGL14.eglChooseConfig(eglDisplay, configAttribs, 0, configs, 0,
        configs.length, numConfigs, 0)) {
      throw new RuntimeException("EGL error " + EGL14.eglGetError());
    }

    mEglConfig = configs[0];

    // EGL 上下文
    int[] attributeList = {
        EGL14.EGL_CONTEXT_CLIENT_VERSION, 2,
        EGL14.EGL_NONE
    };

    // 与GLSurfaceView中的EGLContext共享数据
    mEglContext = EGL14.eglCreateContext(eglDisplay, mEglConfig, mGlContext, attributeList, 0);
    if (EGL14.EGL_NO_CONTEXT == mEglContext) {
      throw new RuntimeException("EGL error " + EGL14.eglGetError());
    }

    // EGLSurface
    int[] surface_attrib_list = {
        EGL14.EGL_NONE
    };
    mEglSurface =
        EGL14.eglCreateWindowSurface(eglDisplay, mEglConfig, surface, surface_attrib_list, 0);
    // mEglSurface == null
    if (mEglSurface == null) {
      throw new RuntimeException("EGL error " + EGL14.eglGetError());
    }

    /*
     * 绑定当前线程的显示器display
     */
    if (!EGL14.eglMakeCurrent(eglDisplay, mEglSurface, mEglSurface, mEglContext)) {
      throw new RuntimeException("EGL error " + EGL14.eglGetError());
    }

    // 绑定当前线程的显示器
    EGL14.eglMakeCurrent(eglDisplay, mEglSurface, mEglSurface, mEglContext);

    recordFilter = new RecordFilter(context);
    FilterContext filterContext = new FilterContext();
    filterContext.setSize(width, height);
    filterChain = new FilterChain(new ArrayList<>(), 0, filterContext);
  }

  public void draw(int textureId, long timestamp) {
    recordFilter.onDraw(textureId, filterChain);
    EGLExt.eglPresentationTimeANDROID(eglDisplay, mEglSurface, timestamp);
    //EGLSurface是双缓冲模式
    EGL14.eglSwapBuffers(eglDisplay, mEglSurface);
  }

  public void release() {
    EGL14.eglDestroySurface(eglDisplay, mEglSurface);
    EGL14.eglMakeCurrent(eglDisplay, EGL14.EGL_NO_SURFACE, EGL14.EGL_NO_SURFACE,
        EGL14.EGL_NO_CONTEXT);
    EGL14.eglDestroyContext(eglDisplay, mEglContext);
    EGL14.eglReleaseThread();
    EGL14.eglTerminate(eglDisplay);
    recordFilter.release();
  }
}
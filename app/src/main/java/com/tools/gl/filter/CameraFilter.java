package com.tools.gl.filter;

import android.content.Context;
import android.opengl.GLES20;
import com.tools.player.R;

public class CameraFilter extends AbstractFboFilter {
  private int vMatrix;
  private float[] mtx;

  public CameraFilter(Context context) {
    super(context, R.raw.camera_vert, R.raw.camera_frag);
  }

  @Override
  public void initGL(Context context, int vertexShaderId, int fragmentShaderId) {
    super.initGL(context, vertexShaderId, fragmentShaderId);
    vMatrix = GLES20.glGetUniformLocation(program, "vMatrix");
  }

  @Override
  public int onDraw(int texture, FilterChain filterChain) {
    mtx = filterChain.filterContext.cameraMtx;
    return super.onDraw(texture, filterChain);
  }

  @Override
  public void beforeDraw() {
    super.beforeDraw();
    GLES20.glUniformMatrix4fv(vMatrix, 1, false, mtx, 0);
  }
}

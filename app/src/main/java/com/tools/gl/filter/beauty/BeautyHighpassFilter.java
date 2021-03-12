package com.tools.gl.filter.beauty;

import android.content.Context;
import android.opengl.GLES20;
import com.tools.gl.filter.AbstractFboFilter;
import com.tools.gl.filter.FilterContext;
import com.tools.player.R;

public class BeautyHighpassFilter extends AbstractFboFilter {
  private int vBlurTexture;
  private int blurTexture;

  public BeautyHighpassFilter(Context context) {
    super(context, R.raw.base_vert, R.raw.beauty_highpass);
  }

  @Override
  public void initGL(Context context, int vertexShaderId, int fragmentShaderId) {
    super.initGL(context, vertexShaderId, fragmentShaderId);
    vBlurTexture = GLES20.glGetUniformLocation(program, "vBlurTexture");
  }

  @Override
  public void beforeDraw(FilterContext filterContext) {
    super.beforeDraw(filterContext);

    GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, blurTexture);
    GLES20.glUniform1i(vBlurTexture, 1);
  }

  public void setBlurTexture(int blurTexture) {
    this.blurTexture = blurTexture;
  }
}

package com.tools.gl.filter;

import android.content.Context;
import android.opengl.GLES20;
import com.tools.player.R;

public class SoulFilter extends AbstractFboFilter {

  private int mixturePercent;
  private int scalePercent;
  float mix = 0.0f;   // 透明度 越大越透明
  float scale = 0.0f; // 缩放   越大越放大

  public SoulFilter(Context context) {
    super(context, R.raw.base_vert, R.raw.soul_frag);
  }

  @Override
  public void initGL(Context context, int vertexShaderId, int fragmentShaderId) {
    super.initGL(context, vertexShaderId, fragmentShaderId);
    mixturePercent = GLES20.glGetUniformLocation(program, "mixturePercent");
    scalePercent = GLES20.glGetUniformLocation(program, "scalePercent");
  }

  @Override
  public void beforeDraw(FilterContext filterContext) {
    super.beforeDraw(filterContext);
    mix += 0.08;
    scale += 0.08;
    if (mix >= 1) {
      mix = 0;
    }
    if (scale >= 1) {
      scale = 0;
    }
    GLES20.glUniform1f(mixturePercent, 1 - mix);
    GLES20.glUniform1f(scalePercent, 1 + scale);
  }
}

package com.tools.gl.filter;

import android.content.Context;
import android.opengl.GLES20;
import com.tools.gl.filter.AbstractFilter;
import com.tools.player.R;

// 将摄像头数据显示
public class ScreenFilter extends AbstractFilter {

  public ScreenFilter(Context context) {
    super(context, R.raw.base_vert, R.raw.base_frag);
  }

}

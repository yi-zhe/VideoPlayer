package com.tools.gl.filter;

import android.content.Context;
import com.tools.player.R;

public class SplitFilter extends AbstractFboFilter {

  public SplitFilter(Context context) {
    super(context, R.raw.base_vert, R.raw.split_frag);
  }
}

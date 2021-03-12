package com.tools.gl.filter;

import android.content.Context;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.opengl.GLES20;
import android.opengl.GLUtils;
import com.tools.cv.Face;
import com.tools.gl.OpenGLUtils;
import com.tools.player.R;

public class StickFilter extends AbstractFboFilter {

  private final int[] textures;

  public StickFilter(Context context) {
    super(context, R.raw.base_vert, R.raw.base_frag);
    textures = new int[1];
    OpenGLUtils.glGenTextures(textures);
    // 图片加载到纹理中
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
    Bitmap bitmap = BitmapFactory.decodeResource(context.getResources(), R.drawable.bizi);
    GLUtils.texImage2D(GLES20.GL_TEXTURE_2D, 0, bitmap, 0);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
  }

  @Override
  public void afterDraw(FilterContext filterContext) {
    super.afterDraw(filterContext);
    // 画鼻子
    Face face = filterContext.face;
    if (face == null) {
      return;
    }

    // 开启混合模式
    GLES20.glEnable(GLES20.GL_BLEND);
    GLES20.glBlendFunc(GLES20.GL_ONE, GLES20.GL_ONE_MINUS_SRC_ALPHA);
    // 计算坐标
    // 基于画布的鼻子的xy
    float x = face.nose_x / face.imgWidth * filterContext.width;
    float y = (1.0f - face.nose_y / face.imgHeight) * filterContext.height;
    // 鼻子贴纸的宽高
    // 通过左右嘴角的x的差 作为鼻子装饰品的宽
    float mrx = face.mouseRight_x / face.imgWidth * filterContext.width;
    float mlx = face.mouseLeft_x / face.imgWidth * filterContext.width;
    int width = (int) (mrx - mlx);

    // 以嘴角的y与鼻子中心点的y的差作为鼻子装饰品的高
    float mry = (1.0f - face.mouseRight_y / face.imgHeight) * filterContext.height;
    int height = (int) (y - mry);

    GLES20.glViewport((int) x - width / 2, (int) y - height / 2, width, height);
    // 画鼻子
    GLES20.glUseProgram(program);

    vertexBuffer.position(0);
    // 4、归一化 normalized  [-1,1] . 把[2,2]转换为[-1,1]
    GLES20.glVertexAttribPointer(vPosition, 2, GLES20.GL_FLOAT, false, 0, vertexBuffer);
    //CPU传数据到GPU，默认情况下着色器无法读取到这个数据。 需要我们启用一下才可以读取
    GLES20.glEnableVertexAttribArray(vPosition);

    textureBuffer.position(0);
    // 4、归一化 normalized  [-1,1] . 把[2,2]转换为[-1,1]
    GLES20.glVertexAttribPointer(vCoord, 2, GLES20.GL_FLOAT, false, 0, textureBuffer);
    //CPU传数据到GPU，默认情况下着色器无法读取到这个数据。 需要我们启用一下才可以读取
    GLES20.glEnableVertexAttribArray(vCoord);

    //相当于激活一个用来显示图片的画框
    GLES20.glActiveTexture(GLES20.GL_TEXTURE1);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, textures[0]);
    // 0: 图层ID  GL_TEXTURE0
    // GL_TEXTURE1 ， 1
    GLES20.glUniform1i(vTexture, 1);

    //通知画画，
    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);

    GLES20.glDisable(GLES20.GL_BLEND);
  }
}

package com.tools.gl;

import android.content.Context;
import android.opengl.GLES20;
import com.tools.player.R;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;
import java.nio.FloatBuffer;

// 将摄像头数据显示
public class ScreenFilter {
  private Context cxt;
  // 顶点坐标
  FloatBuffer vertexBuffer;
  // 纹理坐标
  FloatBuffer textureBuffer;

  private final int vPosition;
  private final int vCoord;
  private final int vTexture;
  private final int vMatrix;
  private int program;
  private int mWidth;
  private int mHeight;
  private float[] mtx = new float[16];

  public ScreenFilter(Context context) {
    this.cxt = context;
    // 准备数据
    vertexBuffer = ByteBuffer.allocateDirect(4 * 4 * 2).order(ByteOrder.nativeOrder())
        .asFloatBuffer();
    textureBuffer = ByteBuffer.allocateDirect(4 * 4 * 2).order(ByteOrder.nativeOrder())
        .asFloatBuffer();

    float[] VERTEX = {
        -1.0f, -1.0f,
        1.0f, -1.0f,
        -1.0f, 1.0f,
        1.0f, 1.0f
    };
    vertexBuffer.clear();
    vertexBuffer.put(VERTEX);
    float[] TEXTURE = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        0.0f, 1.0f,
        1.0f, 1.0f
    };

    textureBuffer.clear();
    textureBuffer.put(TEXTURE);

    String vertexSharder = OpenGLUtils.readRawTextFile(context, R.raw.camera_vert);
    String fragSharder = OpenGLUtils.readRawTextFile(context, R.raw.camera_frag);
    //着色器程序准备好
    program = OpenGLUtils.loadProgram(vertexSharder, fragSharder);

    //获取程序中的变量 索引
    vPosition = GLES20.glGetAttribLocation(program, "vPosition");
    vCoord = GLES20.glGetAttribLocation(program, "vCoord");
    vTexture = GLES20.glGetUniformLocation(program, "vTexture");
    vMatrix = GLES20.glGetUniformLocation(program, "vMatrix");
  }

  public void onDraw(int texture) {
    //设置绘制区域
    GLES20.glViewport(0, 0, mWidth, mHeight);

    GLES20.glUseProgram(program);

    vertexBuffer.position(0);
    // 4、归一化 normalized  [-1,1] . 把[2,2]转换为[-1,1]
    GLES20.glVertexAttribPointer(vPosition, 2, GLES20.GL_FLOAT,
        false, 0, vertexBuffer);
    //CPU传数据到GPU，默认情况下着色器无法读取到这个数据。 需要我们启用一下才可以读取
    GLES20.glEnableVertexAttribArray(vPosition);

    textureBuffer.position(0);
    // 4、归一化 normalized  [-1,1] . 把[2,2]转换为[-1,1]
    GLES20.glVertexAttribPointer(vCoord, 2, GLES20.GL_FLOAT,
        false, 0, textureBuffer);
    //CPU传数据到GPU，默认情况下着色器无法读取到这个数据。 需要我们启用一下才可以读取
    GLES20.glEnableVertexAttribArray(vCoord);

    //相当于激活一个用来显示图片的画框
    GLES20.glActiveTexture(GLES20.GL_TEXTURE0);
    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, texture);
    // 0: 图层ID  glActiveTexture激活的是GL_TEXTURE0 所以用0
    // GL_TEXTURE1 ， 1
    GLES20.glUniform1i(vTexture, 0);

    GLES20.glUniformMatrix4fv(vMatrix, 1, false, mtx, 0);

    //通知画画，
    GLES20.glDrawArrays(GLES20.GL_TRIANGLE_STRIP, 0, 4);

    GLES20.glBindTexture(GLES20.GL_TEXTURE_2D, 0);
  }

  public void setTransformMatrix(float[] mtx) {
    this.mtx = mtx;
  }

  public void setSize(int width, int height) {
    mWidth = width;
    mHeight = height;
  }
}

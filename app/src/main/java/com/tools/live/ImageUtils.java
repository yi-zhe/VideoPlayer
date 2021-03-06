package com.tools.live;

import android.content.Context;
import android.graphics.ImageFormat;
import android.hardware.camera2.CameraAccessException;
import android.hardware.camera2.CameraCharacteristics;
import android.hardware.camera2.CameraManager;
import android.hardware.camera2.params.StreamConfigurationMap;
import android.util.Size;
import androidx.camera.core.ImageProxy;
import java.nio.ByteBuffer;

public class ImageUtils {

  static {
    System.loadLibrary("ImageUtils");
  }

  static ByteBuffer yuv420;
  static byte[] scaleBytes;

  public static byte[] getBytes(ImageProxy image, int rotationDegrees, int width, int height) {
    if (image.getFormat() != ImageFormat.YUV_420_888) {
      // https://developer.android.google.cn/training/camerax/analyze
      throw new IllegalStateException("根据文档，Camerax图像分析返回的就是YUV420!");
    }
    ImageProxy.PlaneProxy[] planes = image.getPlanes();
    // todo 避免内存抖动.
    int size = image.getWidth() * image.getHeight() * 3 / 2;
    if (yuv420 == null || yuv420.capacity() < size) {
      yuv420 = ByteBuffer.allocate(size);
    }
    yuv420.position(0);

    /**
     * Y数据
     */
    ImageProxy.PlaneProxy plane = planes[0];
    //pixelStride = 1 : 取值无间隔
    //pixelStride = 2 : 间隔1个字节取值
    // y的此数据应该都是1
    int pixelStride = plane.getPixelStride();
    //大于等于宽， 表示连续的两行数据的间隔
    //  如：640x480的数据，
    //  可能得到640
    //  可能得到650，表示每行最后10个字节为补位的数据
    int rowStride = plane.getRowStride();
    ByteBuffer buffer = plane.getBuffer();
    byte[] row = new byte[image.getWidth()];
    // 每行要排除的无效数据，但是需要注意：实际测试中 最后一行没有这个补位数据
    byte[] skipRow = new byte[rowStride - image.getWidth()];
    for (int i = 0; i < image.getHeight(); i++) {
      buffer.get(row);
      yuv420.put(row);
      // 不是最后一行
      if (i < image.getHeight() - 1) {
        buffer.get(skipRow);
      }
    }
    /**
     * U V 数据
     */
    for (int i = 1; i < 3; i++) {
      plane = planes[i];
      pixelStride = plane.getPixelStride();
      // uv数据的rowStride可能是
      // 如：640的宽
      // 可能得到320， pixelStride 为1
      // 可能大于320同时小于640，有为了补位的无效数据  pixelStride 为1
      // 可能得到640 uv数据在一起，pixelStride为2
      // 可能大于640，有为了补位的无效数据 pixelStride为2
      rowStride = plane.getRowStride();
      buffer = plane.getBuffer();
      int uvWidth = image.getWidth() / 2;
      int uvHeight = image.getHeight() / 2;

      for (int j = 0; j < uvHeight; j++) {
        for (int k = 0; k < rowStride; k++) {
          // 最后一行，是没有补位数据的
          if (j == uvHeight - 1) {
            //只有自己(U/V)的数据
            if (pixelStride == 1) {
              // 结合外层if 则表示：
              //  如果是最后一行，我们就不管结尾的占位数据了
              if (k >= uvWidth) {
                break;
              }
            } else if (pixelStride == 2) {
              //与同级if相同意思
              // todo uv混合，
              //  planes[2]:uvu
              //  planes[3]:vuv
              if (k >= image.getWidth() - 1) {
                break;
              }
            }
          }
          byte b = buffer.get();
          if (pixelStride == 2) {
            //打包格式 uv在一起,偶数位取出来是U数据： 0 2 4 6
            if (k < image.getWidth() && k % 2 == 0) {
              yuv420.put(b);
            }
          } else if (pixelStride == 1) {
            if (k < uvWidth) {
              yuv420.put(b);
            }
          }
        }
      }
    }
    int srcWidth = image.getWidth();
    int srcHeight = image.getHeight();
    byte[] result = yuv420.array();
    //注意旋转后 宽高变了
    if (rotationDegrees == 90 || rotationDegrees == 270) {
      //todo jni对result修改值，避免内存抖动
      rotation(result, image.getWidth(), image.getHeight(), rotationDegrees);
      srcWidth = image.getHeight();
      srcHeight = image.getWidth();
    }
    if (srcWidth != width || srcHeight != height) {
      //todo jni对scaleBytes修改值，避免内存抖动
      int scaleSize = width * height * 3 / 2;
      if (scaleBytes == null || scaleBytes.length < scaleSize) {
        scaleBytes = new byte[scaleSize];
      }
      scale(result, scaleBytes, srcWidth, srcHeight, width, height);
      return scaleBytes;
    }
    return result;
  }

  private native static void rotation(byte[] data, int width, int height, int rotationDegrees);

  private native static void scale(byte[] src, byte[] dst, int srcWidth, int srcHeight,
      int dstWidth, int dstHeight);

  public static Size[] getSupportSize(Context context) {
    CameraManager cm = (CameraManager) context.getSystemService(Context.CAMERA_SERVICE);
    try {
      for (String cameraId : cm.getCameraIdList()) {
        CameraCharacteristics cameraCharacteristics = cm.getCameraCharacteristics(cameraId);
        StreamConfigurationMap map =
            cameraCharacteristics.get(CameraCharacteristics.SCALER_STREAM_CONFIGURATION_MAP);
        return map.getOutputSizes(ImageFormat.YUV_420_888);
      }
    } catch (CameraAccessException e) {
      e.printStackTrace();
    }
    return null;
  }

  public static byte[] getBytes(ImageProxy image) {
    if (image.getFormat() != ImageFormat.YUV_420_888) {
      // https://developer.android.google.cn/training/camerax/analyze
      throw new IllegalStateException("根据文档，Camerax图像分析返回的就是YUV420!");
    }
    ImageProxy.PlaneProxy[] planes = image.getPlanes();
    // todo 避免内存抖动.
    int size = image.getWidth() * image.getHeight() * 3 / 2;
    if (yuv420 == null || yuv420.capacity() < size) {
      yuv420 = ByteBuffer.allocate(size);
    }
    yuv420.position(0);

    /**
     * Y数据
     */
    ImageProxy.PlaneProxy plane = planes[0];
    //pixelStride = 1 : 取值无间隔
    //pixelStride = 2 : 间隔1个字节取值
    // y的此数据应该都是1
    int pixelStride = plane.getPixelStride();
    //大于等于宽， 表示连续的两行数据的间隔
    //  如：640x480的数据，
    //  可能得到640
    //  可能得到650，表示每行最后10个字节为补位的数据
    int rowStride = plane.getRowStride();
    ByteBuffer buffer = plane.getBuffer();
    byte[] row = new byte[image.getWidth()];
    // 每行要排除的无效数据，但是需要注意：实际测试中 最后一行没有这个补位数据
    byte[] skipRow = new byte[rowStride - image.getWidth()];
    for (int i = 0; i < image.getHeight(); i++) {
      buffer.get(row);
      yuv420.put(row);
      // 不是最后一行
      if (i < image.getHeight() - 1) {
        buffer.get(skipRow);
      }
    }
    /**
     * U V 数据
     */
    for (int i = 1; i < 3; i++) {
      plane = planes[i];
      pixelStride = plane.getPixelStride();
      // uv数据的rowStride可能是
      // 如：640的宽
      // 可能得到320， pixelStride 为1
      // 可能大于320同时小于640，有为了补位的无效数据  pixelStride 为1
      // 可能得到640 uv数据在一起，pixelStride为2
      // 可能大于640，有为了补位的无效数据 pixelStride为2
      rowStride = plane.getRowStride();
      buffer = plane.getBuffer();
      int uvWidth = image.getWidth() / 2;
      int uvHeight = image.getHeight() / 2;

      for (int j = 0; j < uvHeight; j++) {
        for (int k = 0; k < rowStride; k++) {
          // 最后一行，是没有补位数据的
          if (j == uvHeight - 1) {
            //只有自己(U/V)的数据
            if (pixelStride == 1) {
              // 结合外层if 则表示：
              //  如果是最后一行，我们就不管结尾的占位数据了
              if (k >= uvWidth) {
                break;
              }
            } else if (pixelStride == 2) {
              //与同级if相同意思
              if (k >= image.getWidth() - 1) {
                break;
              }
            }
          }
          byte b = buffer.get();
          if (pixelStride == 2) {
            //打包格式 uv在一起,偶数位取出来是U数据： 0 2 4 6
            if (k < image.getWidth() && k % 2 == 0) {
              yuv420.put(b);
            }
          } else if (pixelStride == 1) {
            if (k < uvWidth) {
              yuv420.put(b);
            }
          }
        }
      }
    }
    byte[] result = yuv420.array();
    return result;
  }
}

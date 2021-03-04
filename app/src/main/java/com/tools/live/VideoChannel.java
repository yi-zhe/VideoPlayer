package com.tools.live;

import android.graphics.SurfaceTexture;
import android.os.Handler;
import android.os.HandlerThread;
import android.util.Size;
import android.view.TextureView;
import android.view.ViewGroup;

import androidx.camera.core.CameraX;
import androidx.camera.core.ImageAnalysis;
import androidx.camera.core.ImageAnalysisConfig;
import androidx.camera.core.ImageProxy;
import androidx.camera.core.Preview;
import androidx.camera.core.PreviewConfig;
import androidx.lifecycle.LifecycleOwner;

public class VideoChannel implements ImageAnalysis.Analyzer, Preview.OnPreviewOutputUpdateListener {

    private LifecycleOwner lifecycleOwner;
    private TextureView displayer;
    private RtmpClient rtmpClient;
    private CameraX.LensFacing currentFacing = CameraX.LensFacing.BACK;
    private HandlerThread handlerThread;

    public VideoChannel(LifecycleOwner lifecycleOwner, TextureView displayer, RtmpClient rtmpClient) {
        this.lifecycleOwner = lifecycleOwner;
        this.rtmpClient = rtmpClient;
        this.displayer = displayer;
        //子线程中回调
        handlerThread = new HandlerThread("Analyze-thread");
        handlerThread.start();
        CameraX.bindToLifecycle(lifecycleOwner, getPreView(), getAnalysis());

    }

    private ImageAnalysis getAnalysis() {
        ImageAnalysisConfig imageAnalysisConfig = new ImageAnalysisConfig.Builder()
                .setCallbackHandler(new Handler(handlerThread.getLooper()))
                .setLensFacing(currentFacing)
                .setImageReaderMode(ImageAnalysis.ImageReaderMode.ACQUIRE_LATEST_IMAGE)
                .setTargetResolution(new Size(rtmpClient.getWidth(), rtmpClient.getHeight()))
                .build();
        ImageAnalysis imageAnalysis = new ImageAnalysis(imageAnalysisConfig);
        imageAnalysis.setAnalyzer(this);
        return imageAnalysis;
    }

    private Preview getPreView() {
        // 分辨率并不是最终的分辨率，CameraX会自动根据设备的支持情况，结合你的参数，设置一个最为接近的分辨率
        PreviewConfig previewConfig = new PreviewConfig.Builder()
                .setTargetResolution(new Size(rtmpClient.getWidth(), rtmpClient.getHeight()))
                .setLensFacing(currentFacing) //前置或者后置摄像头
                .build();
        Preview preview = new Preview(previewConfig);
        preview.setOnPreviewOutputUpdateListener(this);
        return preview;
    }

    @Override
    public void analyze(ImageProxy image, int rotationDegrees) {
        // 开启直播并且已经成功连接服务器才获取i420数据
        if (rtmpClient.isConnected()) {
            byte[] bytes = ImageUtils.getBytes(image, rotationDegrees, rtmpClient.getWidth(), rtmpClient.getHeight());
            rtmpClient.sendVideo(bytes);
        }
    }

    @Override
    public void onUpdated(Preview.PreviewOutput output) {
        SurfaceTexture surfaceTexture = output.getSurfaceTexture();
        if (displayer.getSurfaceTexture() != surfaceTexture) {
            if (displayer.isAvailable()) {
                // 当切换摄像头时，会报错
                ViewGroup parent = (ViewGroup) displayer.getParent();
                parent.removeView(displayer);
                parent.addView(displayer, 0);
                parent.requestLayout();
            }
            displayer.setSurfaceTexture(surfaceTexture);
        }
    }

    public void toggleCamera() {
        CameraX.unbindAll();
        if (currentFacing == CameraX.LensFacing.BACK) {
            currentFacing = CameraX.LensFacing.FRONT;
        } else {
            currentFacing = CameraX.LensFacing.BACK;
        }
        CameraX.bindToLifecycle(lifecycleOwner, getPreView(), getAnalysis());
    }

    public void release() {
        handlerThread.quitSafely();
    }
}

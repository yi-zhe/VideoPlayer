//摄像头数据比较特殊的一个地方 如果是png图片则不需要这一行
#extension GL_OES_EGL_image_external : require

precision mediump float;// 数据精度
varying vec2 aCoord;

// 一致变量(常量)从外部初始化 从Java传过来的 uniform 片源着色器中用
uniform samplerExternalOES  vTexture;// samplerExternalOES: 图片， 采样器

void main(){
    //  texture2D: vTexture采样器，采样  aCoord 这个像素点的RGBA值
    vec4 rgba = texture2D(vTexture, aCoord);//rgba
    gl_FragColor = vec4(rgba.r, rgba.g, rgba.b, rgba.a);

}

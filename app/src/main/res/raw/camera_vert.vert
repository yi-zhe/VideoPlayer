// attribute 属性变量、顶点着色器接收数据 从外部加载来的 输入变量(顶点着色器中才可以用)
attribute vec4 vPosition;// (x, y, z, w) 一个顶点

attribute vec2 vCoord;//  纹理坐标

// 易变变量 顶点着色器 输出到 片源着色器
varying vec2 aCoord;

uniform mat4 vMatrix;

void main() {
    //内置变量 把坐标点赋值给gl_position 就Ok了
    gl_Position = vPosition;
    aCoord = (vMatrix * vec4(vCoord, 1.0, 1.0)).xy;
}
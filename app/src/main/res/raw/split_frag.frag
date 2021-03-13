precision mediump float; // 数据精度
varying vec2 aCoord;

uniform sampler2D  vTexture;  // samplerExternalOES: 图片， 采样器

void main(){
    float y = aCoord.y;
    if (y < 0.5) {
        y += 0.25;
    } else {
        y -= 0.25;
    }
    gl_FragColor = texture2D(vTexture, vec2(aCoord.x, y));
}
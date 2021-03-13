varying highp vec2 aCoord;

uniform sampler2D vTexture;
uniform lowp float mixturePercent;
uniform highp float scalePercent;

void main() {
    lowp vec4 textureColor = texture2D(vTexture, aCoord);

    highp vec2 textureCoordinateToUse = aCoord;
    highp vec2 center = vec2(0.5, 0.5);
    textureCoordinateToUse -= center;
    textureCoordinateToUse = textureCoordinateToUse / scalePercent;
    textureCoordinateToUse += center;
    lowp vec4 textureColor2 = texture2D(vTexture, textureCoordinateToUse);

    gl_FragColor = mix(textureColor, textureColor2, mixturePercent);
}

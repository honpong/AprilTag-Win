uniform sampler2D videoFrameY;
uniform sampler2D videoFrameUV;

varying highp vec2 coordinate;

void main()
{
    mediump vec3 yuv;
    lowp vec3 rgb;
    
    yuv.x = texture2D(videoFrameY, coordinate).r;
    yuv.yz = texture2D(videoFrameUV, coordinate).rg - vec2(0.5, 0.5);

    // Using BT.709 which is the standard for HDTV
    rgb = mat3(      1,       1,      1,
                     0, -.18732, 1.8556,
               1.57481, -.46813,      0) * yuv;
    
    gl_FragColor = vec4(rgb, 1);
}



varying lowp vec4 destinationcolor;
varying highp float horizcoord;
varying highp float vertcoord;
lowp float alpha = 0.8;

void main(void) {
    highp float dec = (clamp(abs(horizcoord * 10. - floor(horizcoord * 10. + .5)) * 400., 0.5, 1.5) - .5);
    highp float cm = (clamp(abs(horizcoord * 100. - floor(horizcoord * 100. + .5)) * 40., 0.5, 1.5) - .5) + clamp(sign(vertcoord), 0., 1.);
    highp float mm = (clamp(abs(horizcoord * 1000. - floor(horizcoord * 1000. + .5)) * 4., 0.5, 1.5) - .5) + clamp(sign(vertcoord + .005), 0., 1.);

    mediump vec4 topcolor = vec4(0.73, 0.57, 0., alpha);
    mediump vec4 bottomcolor = vec4(1., 0.78, 0., alpha);
    mediump float blend = clamp(vertcoord * 100., 0., alpha);
    mediump vec4 gradcolor = mix(bottomcolor, topcolor, blend);
    highp float small = min(min(dec, cm), mm);
    gl_FragColor = mix(vec4(0., 0., 0., alpha), gradcolor, small);
}
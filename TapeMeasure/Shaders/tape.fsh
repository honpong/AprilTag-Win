varying lowp vec4 destinationcolor;
varying highp float horizcoord;
varying highp float vertcoord;
lowp float alpha = 0.8;

void main(void) {
    highp float dec = horizcoord * 10. - floor(horizcoord * 10.);
    highp float cm = horizcoord * 100. - floor(horizcoord * 100.);
    highp float mm = horizcoord * 1000. - floor(horizcoord * 1000.);
    if(
       //decimeter
       (dec < .005 || dec > .995)||
       //centimeter
       ((cm < .05 || cm > .95) && vertcoord < 0.)||
       //millimeter
       ((mm < .2 || mm > .98) && vertcoord < -.005)
       ) gl_FragColor = vec4(0., 0., 0., alpha);
    else {
        mediump vec4 topcolor = vec4(0.73, 0.57, 0., alpha);
        mediump vec4 bottomcolor = vec4(1., 0.78, 0., alpha);
        mediump float blend = clamp(vertcoord * 100., 0., alpha);
        gl_FragColor = mix(bottomcolor, topcolor, blend);
    }
}
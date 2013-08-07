varying lowp vec4 destinationcolor;
varying highp float horizcoord;
varying highp float vertcoord;
lowp float alpha = 0.8;

#define FEET_PER_METER 3.28084
#define IN_PER_METER (FEET_PER_METER * 12.)
#define EIGHTS_PER_METER (IN_PER_METER * 8.)

void main(void) {
    highp float dec = (clamp(abs(horizcoord * FEET_PER_METER - floor(horizcoord * FEET_PER_METER + .5)) * (4000. / FEET_PER_METER), 0.5, 1.5) - .5);
    highp float cm = (clamp(abs(horizcoord * IN_PER_METER - floor(horizcoord * IN_PER_METER + .5)) * (4000. / IN_PER_METER), 0.5, 1.5) - .5) + clamp(sign(vertcoord), 0., 1.);
    highp float mm = (clamp(abs(horizcoord * EIGHTS_PER_METER - floor(horizcoord * EIGHTS_PER_METER + .5)) * (4000. / EIGHTS_PER_METER), 0.5, 1.5) - .5) + clamp(sign(vertcoord + .01), 0., 1.);

    mediump vec4 topcolor = vec4(0.73, 0.57, 0., alpha);
    mediump vec4 bottomcolor = vec4(1., 0.78, 0., alpha);
    mediump float blend = clamp(vertcoord * 100., 0., alpha);
    mediump vec4 gradcolor = mix(bottomcolor, topcolor, blend);
    highp float small = min(min(dec, cm), mm);
    gl_FragColor = mix(vec4(0., 0., 0., alpha), gradcolor, small);
}
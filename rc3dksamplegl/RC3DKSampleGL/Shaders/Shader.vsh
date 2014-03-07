//
//  Shader.vsh
//  RC3DKMobileVis
//
//  Created by Brian on 2/28/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

attribute vec4 position;
attribute vec4 color;
attribute vec3 normal;

varying lowp vec4 colorVarying;

uniform mat4 modelViewProjectionMatrix;
uniform mat3 normalMatrix;

void main()
{
    vec3 eyeNormal = normalize(normalMatrix * normal);
    /*
    vec3 lightPosition = vec3(0.0, 0.0, 1.0);
    vec4 diffuseColor = vec4(1.0, 1.0, 1.0, 1.0);
    
    float nDotVP = max(0.0, dot(eyeNormal, normalize(lightPosition)));
                 
    colorVarying = diffuseColor * nDotVP;
     */
    colorVarying = color;
    
    gl_Position = modelViewProjectionMatrix * position;
    gl_PointSize = 8.;
}

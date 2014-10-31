//
//  flat.fsh
//
//  Created by Eagle Jones on 10/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

//uniform lowp vec4 color;

struct directional_light {
    highp vec3 direction;
    highp vec3 halfplane;
    lowp vec4 ambientColor;
    lowp vec4 diffuseColor;
    lowp vec4 specularColor;
};

struct material {
    lowp vec4 ambientFactor;
    lowp vec4 diffuseFactor;
    lowp vec4 specularFactor;
    lowp float shininess;
};

uniform directional_light directional;
uniform material mat;

varying highp vec3 v_normal;

void main()
{
    highp float normaldotlight = max(0., dot(v_normal, directional.direction));
    highp float normaldothalfplane = max(0., dot(v_normal, directional.halfplane));
    lowp vec4 ambient = directional.ambientColor * mat.ambientFactor;
    lowp vec4 diffuse = normaldotlight * directional.diffuseColor * mat.diffuseFactor;
    lowp vec4 specular = vec4(0.);
    if(normaldotlight > 0.) {
        specular = pow(normaldothalfplane, mat.shininess) * directional.specularColor * mat.specularFactor;
    }
    gl_FragColor = ambient + diffuse + specular;
}

//
//  flat.fsh
//
//  Created by Eagle Jones on 10/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

uniform highp vec4 light_ambient;
uniform highp vec4 light_diffuse;
uniform highp vec4 light_specular;

uniform highp vec4 material_ambient;
uniform highp vec4 material_diffuse;
uniform highp vec4 material_specular;
uniform highp float material_shininess;

varying highp vec3 varying_normal;
varying highp vec3 varying_camera_direction;
varying highp vec3 varying_light_direction;

uniform sampler2D texture_value;

varying mediump vec2 coordinate;

void main()
{
    highp vec3 normal = normalize(varying_normal);
    highp vec3 light_direction = normalize(varying_light_direction);
    highp vec3 half_vector = normalize(varying_light_direction + varying_camera_direction);

    highp float diffuse_intensity = max(dot(varying_light_direction, normal), 0.);
    highp float specular_intensity = pow(max(dot(half_vector, normal), 0.), material_shininess);
    
    diffuse_intensity = clamp(diffuse_intensity, 0., 1.);
    specular_intensity = clamp(specular_intensity, 0., 1.);
    
    highp vec4 ambient = light_ambient * texture2D(texture_value, coordinate);
    highp vec4 diffuse = diffuse_intensity * light_diffuse * texture2D(texture_value, coordinate);
    highp vec4 specular = specular_intensity * light_specular * material_specular;
    gl_FragColor = ambient + diffuse + specular; //The ones that should be varying across the surface are not. Why?
}

//
//  flat.vsh
//
//  Created by Eagle Jones on 10/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

attribute vec4 position;
attribute vec3 normal;

uniform vec3 light_direction;

uniform mat4 projection_matrix;
uniform mat4 camera_matrix;
uniform mat4 model_matrix;

varying vec3 varying_normal;
varying vec3 varying_camera_direction;
varying vec3 varying_light_direction;

void main()
{
    gl_Position = projection_matrix * camera_matrix * model_matrix * position;
    //This is not valid if our camera or model matrix is other than rotation, translation, and uniform scaling
    //In that case, use inverse transpose instead
    varying_normal = normalize(vec3(camera_matrix * model_matrix * vec4(normal, 0.)));
    //This could be precomputed, but for point lights we'd do this anyway
    varying_light_direction = normalize(vec3(camera_matrix * vec4(light_direction, 0.)));
    varying_camera_direction = normalize(-vec3(camera_matrix * model_matrix * position));
}

//
//  flat.vsh
//
//  Created by Eagle Jones on 10/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

attribute vec4 position;
attribute vec3 normal;
uniform mat4 projection_matrix;
uniform mat4 camera_matrix;
uniform mat4 model_matrix;

varying vec3 v_normal;

void main()
{
    gl_Position = projection_matrix * camera_matrix * model_matrix * position;
    vec3 norm_calc = vec3(camera_matrix * model_matrix * vec4(normal, 0.));
    v_normal = norm_calc / length(norm_calc);
}

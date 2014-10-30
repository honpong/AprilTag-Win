//
//  flat.vsh
//
//  Created by Eagle Jones on 10/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

attribute vec4 position;
uniform mat4 projection_matrix;
uniform mat4 camera_matrix;
uniform mat4 model_matrix;

void main()
{
    gl_Position = projection_matrix * camera_matrix * model_matrix * position;
}

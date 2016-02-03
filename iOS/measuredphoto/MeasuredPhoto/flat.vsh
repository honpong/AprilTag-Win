//
//  flat.vsh
//
//  Created by Eagle Jones on 10/1/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

attribute vec4 position;
uniform mat4 projection_matrix;
uniform mat4 camera_matrix;
uniform mat4 camera_screen_transform;

void main()
{
    gl_Position = camera_screen_transform * projection_matrix * camera_matrix * position;
}

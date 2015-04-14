//
//  Shader.fsh
//  RC3DKMobileVis
//
//  Created by Brian on 2/28/14.
//  Copyright (c) 2014 Realitycap. All rights reserved.
//

varying lowp vec4 colorVarying;

void main()
{
    gl_FragColor = colorVarying;
}

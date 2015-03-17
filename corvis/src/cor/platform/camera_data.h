//
//  camera_data.h
//  RC3DK
//
//  Created by Eagle Jones on 3/17/15.
//  Copyright (c) 2015 RealityCap. All rights reserved.
//

#ifndef __RC3DK__camera_data__
#define __RC3DK__camera_data__

#include <memory>

class camera_data
{
public:
    camera_data();
    camera_data(void *handle);
    ~camera_data();
    camera_data(camera_data&& other) = default;
    camera_data &operator=(camera_data&& other) = default;
    
    std::unique_ptr<void, void(*)(void *)> image_handle;
    uint64_t timestamp;
    uint8_t *image;
    int width, height, stride;
};

#endif /* defined(__RC3DK__camera_data__) */

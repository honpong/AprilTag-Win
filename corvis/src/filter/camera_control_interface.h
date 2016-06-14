//
//  camera_control_interface.h
//  
//
//  Created by Eagle Jones on 12/11/14.
//  Copyright (c) 2014 RealityCap. All rights reserved.
//
//  C++ Interface file allowing corvis to call platform-specific camera control implementation code (included in both corvis and platform-specific worlds)

#include <functional>

#ifndef __camera_control_interface_h
#define __camera_control_interface_h

class camera_control_interface
{
public:
    camera_control_interface();
    ~camera_control_interface();
    
    /**
     Initializes the internal implementation with some platform-specific object.
     For Objective C, this should an AVCaptureDevice, passed using (__bridge void *)device; the AVCaptureDevice will be retained/released within the implementation.
     */
    void init(void *platform_specific_object);
    
    /**
     Releases the platform specific object.
     */
    //TODO: get rid of this? Only included since the previous implementation required it; not sure if it makes sense here
    void release_platform_specific_object();
    
    /**
     Locks focus at the current position.
     
     If a focus action is already in process, allows it to complete, but does not trigger a new one if it's not already in process.
     
     @param callback - A lambda that will be called after the focus has successfully been locked. It receives the time at which the focus operation completed (frames delivered at this time or later are expected to be post-focus operation), and a float value indicating the final lens position in the range [0, 1], corresponding to [near, infinity].
     */
    void focus_lock_at_current_position(std::function<void (uint64_t, float)> callback);
    
    /**
     Locks focus at the given position.
     
     @param position - A float value indicating the final lens position in the range [0, 1], corresponding to [near, infinity].
     @param callback - A lambda that will be called after the focus has successfully been locked. It receives the time at which the focus operation completed (frames delivered at this time or later are expected to be post-focus operation).
     */
    void focus_lock_at_position(float position, std::function<void (uint64_t, float)> callback);

    /**
     Performs a single autofocus operation, then locks the focus.
     
     Regardless of the current focus state (locked or unlocked), triggers a single new autofocus, after which the focus is locked.
     
     @param callback - A lambda that will be called after the focus has successfully been locked. It receives the time at which the focus operation completed (frames delivered at this time or later are expected to be post-focus operation), and a float value indicating the final lens position in the range [0, 1], corresponding to [near, infinity].
     */
    void focus_once_and_lock(std::function<void (uint64_t, float)> callback);
    
    /**
     Unlocks the focus.
     
     After this method is called, the focus should behave as it does in a typical camera app; essentially refocusing when needed, but not hunting constantly.
     */
    void focus_unlock();
    
private:
    void *platform_ptr; // this contains a pointer to whatever platform-specific object we need to talk to
};

#endif

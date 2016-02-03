#ifndef __CORVIS_WORLD_STATE_RENDER_H__
#define __CORVIS_WORLD_STATE_RENDER_H__

#include "world_state.h"

// Assumes you have already configured OpenGL for your platform
// Compiles vertex and fragment shaders and does other render setup
// Returns false if anything fails to compile
bool world_state_render_init();
// Deletes the compiled shader program
void world_state_render_teardown();
// Render the current world state, passing the modelview projection matrix and the projection matrix to the shaders
void world_state_render(world_state * world, float * _modelViewMatrix, float * _projectionMatrix);
bool world_state_render_video_init();
void world_state_render_video(world_state * world, int viewport_width, int viewport_height);
bool world_state_render_video_get_size(world_state * world, int *video_width, int *video_height);
void world_state_render_video_teardown();
bool world_state_render_depth_init();
void world_state_render_depth(world_state * world, int viewport_width, int viewport_height);
bool world_state_render_depth_get_size(world_state * world, int *video_width, int *video_height);
void world_state_render_depth_teardown();
bool world_state_render_plot_init();
void world_state_render_plot(world_state * world, size_t plot_index, size_t key_index, int viewport_width, int viewport_height);
void world_state_render_plot_teardown();


#endif

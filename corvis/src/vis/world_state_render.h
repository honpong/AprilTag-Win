#ifndef __CORVIS_WORLD_STATE_RENDER_H__
#define __CORVIS_WORLD_STATE_RENDER_H__

#include "world_state.h"

// Assumes you have already configured OpenGL for your platform
// Compiles vertex and fragment shaders and does other render setup
// Returns false if anything fails to compile
bool world_state_render_init();
// Deletes the compiled shader program
void world_state_render_teardown();
// Render the current world state, passing the modelview projection matrix and the normal matrix to the shaders
void world_state_render(world_state & world, float * _modelViewProjectionMatrix, float * _normalMatrix);

#endif

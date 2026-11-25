#pragma once

#include <SDL3/SDL.h>

namespace alpha::platform {

// Lets the GPU driver queue at most one frame ahead of the screen, so input
// shows up a frame or two sooner. Without this, a vsynced Direct3D 11 swap
// chain may queue up to 3 frames (~50 ms of input lag at 60 Hz).
// Returns true if the renderer's backend supports it and it was applied
// (Windows / direct3d11); other backends are left unchanged.
bool limit_frame_queue(SDL_Renderer* renderer);

}  // namespace alpha::platform

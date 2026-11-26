#include "platform/frame_latency.hpp"

#ifdef _WIN32
#include <d3d11.h>
#include <dxgi.h>
#endif

namespace alpha::platform {

#ifdef _WIN32

namespace {

bool set_max_latency(ID3D11Device* device) {
  IDXGIDevice1* dxgi = nullptr;
  if (FAILED(device->QueryInterface(__uuidof(IDXGIDevice1), reinterpret_cast<void**>(&dxgi)))) {
    return false;
  }
  const bool ok = SUCCEEDED(dxgi->SetMaximumFrameLatency(1));
  dxgi->Release();
  return ok;
}

}  // namespace

bool limit_frame_queue(SDL_Renderer* renderer) {
  const SDL_PropertiesID props = SDL_GetRendererProperties(renderer);
  auto* device = static_cast<ID3D11Device*>(
      SDL_GetPointerProperty(props, SDL_PROP_RENDERER_D3D11_DEVICE_POINTER, nullptr));
  return device != nullptr && set_max_latency(device);
}

#else

bool limit_frame_queue(SDL_Renderer*) { return false; }

#endif

void limit_latency(SDL_Renderer* renderer, bool vsync) {
  SDL_SetRenderVSync(renderer, vsync ? 1 : SDL_RENDERER_VSYNC_DISABLED);
  [[maybe_unused]] const bool short_queue = limit_frame_queue(renderer);
#if ALPHA_DEBUG_TOOLS  // diagnostics
  SDL_Log("renderer %s, frame queue: %s", SDL_GetRendererName(renderer), short_queue ? "1 frame" : "driver default");
#endif
}

}  // namespace alpha::platform

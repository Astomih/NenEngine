#include <SDL.h>
#include <Time/Time.hpp>

namespace nen {
float time::GetTicksAsSeconds() {
  return static_cast<float>(SDL_GetTicks() / 1000.f);
}

uint32_t time::GetTicksAsMilliSeconds() { return SDL_GetTicks(); }
} // namespace nen
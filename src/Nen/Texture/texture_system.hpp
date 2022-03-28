#pragma once
#include <SDL.h>
#include <Utility/handler.hpp>
#include <memory>
#include <string_view>
#include <unordered_map>

namespace nen {
struct SDLObjectCloser {
  void operator()(::SDL_Surface *surface);
  void operator()(::SDL_RWops *rw);
};
class texture_system {
public:
  texture_system(class manager &_manager) : m_manager(_manager) {}
  handle_t create();
  bool Contain(handle_t);
  SDL_Surface &get(handle_t);
  SDL_Surface *get_raw(handle_t);
  void move(handle_t, std::unique_ptr<SDL_Surface, SDLObjectCloser>);
  void remove(handle_t);

private:
  class manager &m_manager;
  handler<::SDL_Surface, SDLObjectCloser> surfaces;
};
} // namespace nen

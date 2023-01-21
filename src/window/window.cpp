#include "../event/event_system.hpp"
#include "window_system.hpp"
#include <SDL.h>
#include <io/file.hpp>
#include <io/json.hpp>
#include <window/window.hpp>

namespace sinen {
vector2 window_system::m_size = vector2(1280.f, 720.f);
std::string window_system::m_name = "Sinen Engine";
window_state window_system::m_state = window_state::ENTER;
::SDL_Window *window_system::m_window = nullptr;
const void *window::get_sdl_window() { return window_system::get_sdl_window(); }
vector2 window::size() { return window_system::size(); }
vector2 window::center() { return window_system::center(); }
void window::set_size(const vector2 &size) { window_system::set_size(size); }
void window::set_fullscreen(bool fullscreen) {
  window_system::set_fullscreen(fullscreen);
}
void window::set_name(const std::string &name) {
  window_system::set_name(name);
}
std::string window::name() { return window_system::name(); }
const window_state &window::state() { return window_system::state(); }

void window_system::initialize(const std::string &name, graphics_api api) {
  m_name = name;

#if !defined(EMSCRIPTEN) && !defined(MOBILE)
  {
    file f;
    f.open("settings.json", file::mode::r);
    void *buffer = calloc(f.size() + 10, 1);
    f.read(buffer, f.size(), 1);
    f.close();
    json j;
    j.parse((char *)buffer);
    m_size.x = j["WindowWidth"].get_float();
    m_size.y = j["WindowHeight"].get_float();
  }

  switch (api) {
  case graphics_api::Vulkan: {
    m_window = SDL_CreateWindow(
        std::string(name).c_str(), SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, static_cast<int>(m_size.x),
        static_cast<int>(m_size.y), SDL_WINDOW_VULKAN | SDL_WINDOW_SHOWN);

    break;
  }
  case graphics_api::OpenGL: {
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,
                        SDL_GL_CONTEXT_PROFILE_CORE);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
    SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
    SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
    m_window = SDL_CreateWindow(
        std::string(name).c_str(), SDL_WINDOWPOS_UNDEFINED,
        SDL_WINDOWPOS_UNDEFINED, static_cast<int>(m_size.x),
        static_cast<int>(m_size.y), SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
    break;
  }
  default:
    break;
  }
#endif
#if defined(EMSCRIPTEN) || defined(MOBILE)
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_ES);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 0);
  SDL_GL_SetAttribute(SDL_GL_RED_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, 8);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, 1);
  m_window = SDL_CreateWindow(
      std::string(m_name).c_str(), SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, static_cast<int>(m_size.x),
      static_cast<int>(m_size.y), SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
#endif
}

void window_system::shutdown() {
  SDL_DestroyWindow(m_window);
  m_window = nullptr;
}
void window_system::set_size(const vector2 &size) {
  m_size = size;
  SDL_SetWindowSize(m_window, static_cast<int>(m_size.x),
                    static_cast<int>(m_size.y));
}
void window_system::set_fullscreen(bool fullscreen) {
  SDL_SetWindowFullscreen(m_window,
                          fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP: 0);
}
void window_system::set_name(const std::string &name) {
  m_name = name;
  SDL_SetWindowTitle(m_window, m_name.c_str());
}

void window_system::process_input() {
  int x, y;
  SDL_GetWindowSize(m_window, &x, &y);
  m_size.x = static_cast<float>(x);
  m_size.y = static_cast<float>(y);
  if (event_system::current_event.type == SDL_WINDOWEVENT) {
    m_state =
        static_cast<window_state>(event_system::current_event.window.event);
  }
}
} // namespace sinen

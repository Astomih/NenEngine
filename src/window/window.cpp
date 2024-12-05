#include "window_system.hpp"
#include <SDL.h>
#include <input/keyboard.hpp>
#include <io/file.hpp>
#include <io/json.hpp>
#include <window/window.hpp>

namespace sinen {
Vector2 WindowImpl::m_size = Vector2(1280.f, 720.f);
std::string WindowImpl::m_name = "Sinen Engine";
::SDL_Window *WindowImpl::m_window = nullptr;
bool WindowImpl::m_resized = false;
const void *Window::get_sdl_window() { return WindowImpl::get_sdl_window(); }
Vector2 Window::size() { return WindowImpl::size(); }
Vector2 Window::center() { return WindowImpl::center(); }
void Window::resize(const Vector2 &size) { WindowImpl::resize(size); }
void Window::set_fullscreen(bool fullscreen) {
  WindowImpl::set_fullscreen(fullscreen);
}
void Window::rename(const std::string &name) { WindowImpl::rename(name); }
std::string Window::name() { return WindowImpl::name(); }
bool Window::resized() { return WindowImpl::resized(); }

void WindowImpl::initialize(const std::string &name) {
  m_name = name;

  // Load settings from settings.json
  {
    File f;
    if (f.open("settings.json", File::mode::r)) {

      void *buffer = calloc(f.size() + 10, 1);
      f.read(buffer, f.size(), 1);
      f.close();
      Json j;
      j.parse((char *)buffer);
      m_size.x = j["WindowWidth"].get_float();
      m_size.y = j["WindowHeight"].get_float();
    } else {
      f.close();
      m_size.x = 1280;
      m_size.y = 720;
    }
  }

  m_window = SDL_CreateWindow(
      std::string(name).c_str(), SDL_WINDOWPOS_UNDEFINED,
      SDL_WINDOWPOS_UNDEFINED, static_cast<int>(m_size.x),
      static_cast<int>(m_size.y), SDL_WINDOW_VULKAN | SDL_WINDOW_RESIZABLE);
}

void WindowImpl::shutdown() {
  SDL_DestroyWindow(m_window);
  m_window = nullptr;
}
void WindowImpl::resize(const Vector2 &size) {
  m_size = size;
  SDL_SetWindowSize(m_window, static_cast<int>(m_size.x),
                    static_cast<int>(m_size.y));
}
void WindowImpl::set_fullscreen(bool fullscreen) {
  SDL_SetWindowFullscreen(m_window,
                          fullscreen ? SDL_WINDOW_FULLSCREEN_DESKTOP : 0);
}
void WindowImpl::rename(const std::string &name) {
  m_name = name;
  SDL_SetWindowTitle(m_window, m_name.c_str());
}
void WindowImpl::prepare_frame() { m_resized = false; }
void WindowImpl::process_input(SDL_Event &event) {
  int x, y;
  SDL_GetWindowSize(m_window, &x, &y);
  m_size.x = static_cast<float>(x);
  m_size.y = static_cast<float>(y);
  if (event.type == SDL_WINDOWEVENT) {
    if (event.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
      m_resized = true;
    }
  }
  if (Keyboard::is_pressed(Keyboard::code::F11)) {
    static bool fullscreen = false;
    fullscreen = !fullscreen;
    set_fullscreen(fullscreen);
  }
}
} // namespace sinen

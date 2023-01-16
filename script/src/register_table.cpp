#include "register_script.hpp"

#include <render/renderer.hpp>
#include <scene/scene.hpp>
#include <window/window.hpp>

namespace sinen {
void register_table(sol::state &lua) {
  {
    auto v = lua.create_table("window");
    v["name"] = &window::name;
    v["state"] = []() { return static_cast<int>(window::state()); };
    v["size"] = &window::size;
    v["center"] = window::center;
    v["set_size"] = &window::set_size;
    v["set_fullscreen"] = &window::set_fullscreen;
    v["set_name"] = &window::set_name;
  }
  {
    auto v = lua.create_table("renderer");
    v["set_skybox"] = &renderer::set_skybox;
    v["skybox"] = &renderer::skybox;
    v["clear_color"] = &renderer::clear_color;
    v["set_clear_color"] = &renderer::set_clear_color;
    v["get_graphics_api"] = &renderer::get_graphics_api;
  }
  {
    auto v = lua.create_table("scene");
    v["load"] = &scene::load;
    v["main_camera"] = &scene::main_camera;
    v["get_actor"] = &scene::get_actor;
  }
  {
    auto v = lua.create_table("collision");
    v["aabb_aabb"] = &collision::aabb_aabb;
  }
}
} // namespace sinen

#include "render_system.hpp"
#include <math/vector3.hpp>
#include <render/renderer.hpp>
#include <vertex/vertex.hpp>
#include <vertex/vertex_array.hpp>

namespace sinen {
graphics_api renderer::get_graphics_api() {
  return render_system::get_graphics_api();
}

void renderer::unload_data() { render_system::unload_data(); }

void renderer::render() { render_system::render(); }

void renderer::draw2d(const std::shared_ptr<drawable> draw_object) {
  render_system::draw2d(draw_object);
}
void renderer::drawui(const std::shared_ptr<drawable> draw_object) {
  render_system::drawui(draw_object);
}
void renderer::draw3d(const std::shared_ptr<drawable> draw_object) {
  render_system::draw3d(draw_object);
}
void renderer::add_queue_2d(const std::shared_ptr<drawable> draw_object) {
  render_system::add_queue_2d(draw_object);
}
void renderer::add_queue_3d(const std::shared_ptr<drawable> draw_object) {
  render_system::add_queue_3d(draw_object);
}
void renderer::add_queue_instancing(const std::shared_ptr<instancing> object) {
  render_system::add_queue_instancing(object);
}
void renderer::remove_queue_2d(const std::shared_ptr<drawable> draw_object) {
  render_system::remove_queue_2d(draw_object);
}
void renderer::remove_queue_3d(const std::shared_ptr<drawable> draw_object) {
  render_system::remove_queue_3d(draw_object);
}
void renderer::remove_queue_instancing(
    const std::shared_ptr<instancing> object) {
  render_system::remove_queue_instancing(object);
}
void renderer::add_vertex_array(const vertex_array &vArray,
                                std::string_view name) {
  render_system::add_vertex_array(vArray, name);
}
void renderer::update_vertex_array(const vertex_array &vArray,
                                   std::string_view name) {
  render_system::update_vertex_array(vArray, name);
}
void renderer::add_model(const model &_model) {
  render_system::add_model(_model);
}
void renderer::update_model(const model &_model) {
  render_system::update_model(_model);
}

void renderer::add_instancing(const instancing &_instancing) {
  render_system::add_instancing(_instancing);
}

void renderer::set_clear_color(const color &color) {
  render_system::set_clear_color(color);
}

color renderer::clear_color() { return render_system::get_clear_color(); }

void renderer::set_skybox(texture _skybox_texture) {
  render_system::set_skybox_texture(_skybox_texture);
}

texture renderer::skybox() { return render_system::get_skybox_texture(); }

void renderer::toggle_show_imgui() { render_system::toggle_show_imgui(); }
bool renderer::is_show_imgui() { return render_system::is_show_imgui(); }

void renderer::load_shader(const shader &shaderinfo) {
  render_system::load_shader(shaderinfo);
}
void renderer::unload_shader(const shader &shaderinfo) {
  render_system::unload_shader(shaderinfo);
}

std::list<std::function<void()>> &renderer::get_imgui_function() {
  return render_system::get_imgui_function();
}

void renderer::add_imgui_function(std::function<void()> function) {
  render_system::get_imgui_function().push_back(function);
}
} // namespace sinen

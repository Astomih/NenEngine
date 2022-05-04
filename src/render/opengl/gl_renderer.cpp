#include <manager/manager.hpp>

#include <SDL.h>
#include <SDL_image.h>

#if defined(EMSCRIPTEN) || defined(MOBILE)
#include <GLES3/gl3.h>
#include <SDL_opengles2.h>
#endif

#ifndef MOBILE
#include <GL/glew.h>
#endif

#include <cstdint>
#include <vector>

#include "../../texture/texture_system.hpp"
#include "gl_renderer.hpp"
#include <camera/camera.hpp>
#include <fstream>
#include <imgui.h>
#include <imgui_impl_opengl3.h>
#include <imgui_impl_sdl.h>
#include <io/dstream.hpp>
#include <iostream>
#include <logger/logger.hpp>
#include <sstream>
#include <window/window.hpp>

namespace nen::gl {
gl_renderer::gl_renderer(manager &_manager) : m_manager(_manager) {}

void gl_renderer::Initialize() {
  auto &w = m_manager.get_window();
  mContext = SDL_GL_CreateContext((SDL_Window *)w.GetSDLWindow());
  SDL_GL_MakeCurrent((SDL_Window *)w.GetSDLWindow(), mContext);
  prev_window_x = w.Size().x;
  prev_window_y = w.Size().y;
#if !defined EMSCRIPTEN && !defined MOBILE
  glewExperimental = GL_TRUE;
  if (glewInit() != GLEW_OK) {
    logger::error("GLEW Init error.");
  }
  glGetError();
#endif

  prepare();
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  (void)io;
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
  io.ConfigFlags |=
      ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
  io.IniFilename = NULL;
  io.Fonts->AddFontFromFileTTF("data/font/mplus/mplus-1p-medium.ttf", 18.0f,
                               nullptr, io.Fonts->GetGlyphRangesJapanese());
  ImGui_ImplSDL2_InitForOpenGL(
      (SDL_Window *)m_manager.get_window().GetSDLWindow(), mContext);
#if defined EMSCRIPTEN || defined MOBILE
  ImGui_ImplOpenGL3_Init("#version 300 es");
#else
  ImGui_ImplOpenGL3_Init("#version 330 core");
#endif
}

void gl_renderer::Render() {
  auto &w = m_manager.get_window();
  if (w.Size().x != prev_window_x || w.Size().y != prev_window_y) {
    logger::info("window size.x: ", w.Size().x, " window size.y: ", w.Size().y);
    glViewport(0, 0, w.Size().x, w.Size().y);
    prev_window_x = w.Size().x;
    prev_window_y = w.Size().y;
  }
  auto color = m_manager.get_renderer().GetClearColor();
  glClearColor(color.r, color.g, color.b, 1.0);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  glEnable(GL_BLEND);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
  draw_skybox();
  glDisable(GL_BLEND);
  glEnable(GL_DEPTH_TEST);
  draw_3d();
  draw_instancing_3d();
  glDisable(GL_DEPTH_TEST);
  glEnable(GL_BLEND);
  glBlendEquationSeparate(GL_FUNC_ADD, GL_FUNC_ADD);
  glBlendFuncSeparate(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA, GL_ONE, GL_ZERO);
  draw_2d();
  draw_instancing_2d();
  glDisable(GL_BLEND);
  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame((SDL_Window *)w.GetSDLWindow());
  ImGui::NewFrame();
  if (m_manager.get_renderer().isShowImGui()) {
    ImGui::SetNextWindowPos(ImVec2(0, 0), ImGuiCond_Always);
    ImGui::SetNextWindowSize(
        ImVec2(get_window().Size().x, get_window().Size().y), ImGuiCond_Always);

    // Draw ImGUI widgets.
    ImGui::Begin("Editor", nullptr,
                 ImGuiWindowFlags_HorizontalScrollbar |
                     ImGuiWindowFlags_MenuBar);
    for (auto &i : get_renderer().get_imgui_function()) {
      i();
    }
    ImGui::End();
  }

  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  mSprite3Ds.clear();
  mSprite2Ds.clear();
  for (auto &i : m_instancing_2d) {
    glDeleteBuffers(1, &i.vbo);
  }
  for (auto &i : m_instancing_3d) {
    glDeleteBuffers(1, &i.vbo);
  }
  m_instancing_3d.clear();
  m_instancing_2d.clear();
  SDL_GL_SwapWindow((SDL_Window *)w.GetSDLWindow());
}

void gl_renderer::enable_vertex_attrib_array() {
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glEnableVertexAttribArray(6);
  glEnableVertexAttribArray(7);
}

void gl_renderer::disable_vertex_attrib_array() {
  glDisableVertexAttribArray(4);
  glDisableVertexAttribArray(5);
  glDisableVertexAttribArray(6);
  glDisableVertexAttribArray(7);
}

void gl_renderer::draw_skybox() {
  registerTexture(get_renderer().skybox_texture->handle);
  auto &va = m_VertexArrays["BOX"];
  glBindVertexArray(va.vao);
  disable_vertex_attrib_array();
  mSpriteShader.SetActive(0);
  shader_parameter param;
  matrix4 w = matrix4::Identity;
  w[0][0] = 5;
  w[1][1] = 5;
  w[2][2] = 5;
  param.proj = get_camera().projection;
  param.view = matrix4::LookAt(vector3(0, 0, 0),
                               get_camera().target - get_camera().position,
                               get_camera().up);
  mSpriteShader.SetActive(0);
  mSpriteShader.UpdateUBO(0, sizeof(shader_parameter), &param);
  glBindTexture(GL_TEXTURE_2D,
                mTextureIDs[get_renderer().skybox_texture->handle]);
  glDrawElements(GL_TRIANGLES, va.indices.size(), GL_UNSIGNED_INT, nullptr);
}

void gl_renderer::draw_3d() {
  for (auto &i : mSprite3Ds) {
    auto &va = m_VertexArrays[i->vertexIndex];
    glBindVertexArray(va.vao);
    disable_vertex_attrib_array();
    if (i->shader_data.vertName == "default" &&
        i->shader_data.fragName == "default") {
      mSpriteShader.SetActive(0);
      mSpriteShader.UpdateUBO(0, sizeof(shader_parameter), &i->param);
    } else {
      for (auto &j : userPipelines) {
        if (j.first == i->shader_data) {
          j.second.SetActive(0);
          j.second.UpdateUBO(0, sizeof(shader_parameter), &i->param);
        }
      }
    }
    glBindTexture(GL_TEXTURE_2D, mTextureIDs[i->texture_handle]);
    glDrawElements(GL_TRIANGLES, m_VertexArrays[i->vertexIndex].indices.size(),
                   GL_UNSIGNED_INT, nullptr);
  }
}
void gl_renderer::draw_instancing_3d() {
  for (auto &i : m_instancing_3d) {
    mSpriteInstanceShader.SetActive(0);
    mSpriteInstanceShader.UpdateUBO(0, sizeof(shader_parameter),
                                    &i.ins.object->param);

    auto &va = m_VertexArrays[i.ins.object->vertexIndex];
    glBindVertexArray(va.vao);
    enable_vertex_attrib_array();

    glBindTexture(GL_TEXTURE_2D, mTextureIDs[i.ins.object->texture_handle]);
    glDrawElementsInstanced(
        GL_TRIANGLES, m_VertexArrays[i.ins.object->vertexIndex].indices.size(),
        GL_UNSIGNED_INT, nullptr, i.ins.data.size());
  }
}

void gl_renderer::draw_2d() {
  for (auto &i : mSprite2Ds) {
    glBindVertexArray(m_VertexArrays[i->vertexIndex].vao);
    disable_vertex_attrib_array();
    glBindTexture(GL_TEXTURE_2D, mTextureIDs[i->texture_handle]);
    if (i->shader_data.vertName == "default" &&
        i->shader_data.fragName == "default") {
      mAlphaShader.SetActive(0);
      mAlphaShader.UpdateUBO(0, sizeof(shader_parameter), &i->param);
    } else {
      for (auto &j : userPipelines) {
        if (j.first == i->shader_data) {
          j.second.SetActive(0);
          j.second.UpdateUBO(0, sizeof(shader_parameter), &i->param);
        }
      }
    }
    glDrawElements(GL_TRIANGLES, m_VertexArrays[i->vertexIndex].indices.size(),
                   GL_UNSIGNED_INT, nullptr);
  }
}

void gl_renderer::draw_instancing_2d() {
  for (auto &i : m_instancing_2d) {
    mAlphaInstanceShader.SetActive(0);
    mAlphaInstanceShader.UpdateUBO(0, sizeof(shader_parameter),
                                   &i.ins.object->param);

    auto &va = m_VertexArrays[i.ins.object->vertexIndex];
    glBindVertexArray(va.vao);
    enable_vertex_attrib_array();

    glBindTexture(GL_TEXTURE_2D, mTextureIDs[i.ins.object->texture_handle]);
    glDrawElementsInstanced(
        GL_TRIANGLES, m_VertexArrays[i.ins.object->vertexIndex].indices.size(),
        GL_UNSIGNED_INT, nullptr, i.ins.data.size());
  }
}

void gl_renderer::AddVertexArray(const vertex_array &vArray,
                                 std::string_view name) {
  gl::gl_vertex_array vArrayGL;
  vArrayGL.indexCount = vArray.indexCount;
  vArrayGL.indices = vArray.indices;
  vArrayGL.vertices = vArray.vertices;
  vArrayGL.materialName = vArray.materialName;

  // Create vao
  glGenVertexArrays(1, &vArrayGL.vao);
  glBindVertexArray(vArrayGL.vao);

  // Create vbo
  glGenBuffers(1, &vArrayGL.vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vArrayGL.vbo);
  auto vArraySize = vArrayGL.vertices.size() * sizeof(vertex);
  glBufferData(GL_ARRAY_BUFFER, vArraySize, vArrayGL.vertices.data(),
               GL_DYNAMIC_DRAW);

  // Prepare vertex attribute
  glBindBuffer(GL_ARRAY_BUFFER, vArrayGL.vbo);
  glEnableVertexAttribArray(0);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float), 0);
  glEnableVertexAttribArray(1);
  glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 12 * sizeof(float),
                        reinterpret_cast<void *>(sizeof(float) * 3));
  glEnableVertexAttribArray(2);
  glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 12 * sizeof(float),
                        reinterpret_cast<void *>(sizeof(float) * 6));
  glEnableVertexAttribArray(3);
  glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, 12 * sizeof(float),
                        reinterpret_cast<void *>(sizeof(float) * 8));
  // Create IBO
  glGenBuffers(1, &vArrayGL.ibo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vArrayGL.ibo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER,
               vArrayGL.indices.size() * sizeof(uint32_t),
               vArrayGL.indices.data(), GL_DYNAMIC_DRAW);
  m_VertexArrays.emplace(std::string(name), vArrayGL);
}
void gl_renderer::UpdateVertexArray(const vertex_array &vArray,
                                    std::string_view name) {
  gl::gl_vertex_array vArrayGL;
  vArrayGL.indexCount = vArray.indexCount;
  vArrayGL.indices = vArray.indices;
  vArrayGL.vertices = vArray.vertices;
  vArrayGL.materialName = vArray.materialName;
  // vboを更新
  glBindBuffer(GL_ARRAY_BUFFER, m_VertexArrays[name.data()].vbo);
  auto vArraySize = vArrayGL.vertices.size() * sizeof(vertex);
  glBufferSubData(GL_ARRAY_BUFFER, 0, vArraySize, vArrayGL.vertices.data());
  // iboを更新
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_VertexArrays[name.data()].ibo);
  glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0,
                  vArrayGL.indices.size() * sizeof(uint32_t),
                  vArrayGL.indices.data());
}

void gl_renderer::draw2d(std::shared_ptr<class draw_object> sprite) {
  registerTexture(sprite->texture_handle);
  pushSprite2d(sprite);
}

void gl_renderer::draw3d(std::shared_ptr<class draw_object> sprite) {
  registerTexture(sprite->texture_handle);
  pushSprite3d(sprite);
}

void gl_renderer::LoadShader(const shader &shaderInfo) {
  gl_shader pipeline;
  pipeline.load(shaderInfo.vertName, shaderInfo.fragName);
  shader_parameter param;
  pipeline.create_ubo(0, sizeof(shader_parameter), &param);
  userPipelines.emplace_back(
      std::pair<shader, gl_shader>{shaderInfo, pipeline});
}

void gl_renderer::UnloadShader(const shader &shaderInfo) {
  std::erase_if(userPipelines, [&](auto &x) {
    if (x.first == shaderInfo) {
      x.second.unload();
      return true;
    };
    return false;
  });
}

void gl_renderer::add_instancing(const instancing &_instancing) {
  registerTexture(_instancing.object->texture_handle);
  auto &va = m_VertexArrays[_instancing.object->vertexIndex];
  glBindVertexArray(va.vao);
  uint32_t vbo;
  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, _instancing.size, _instancing.data.data(),
               GL_DYNAMIC_DRAW);
  auto size = sizeof(instance_data);
  glEnableVertexAttribArray(4);
  glEnableVertexAttribArray(5);
  glEnableVertexAttribArray(6);
  glEnableVertexAttribArray(7);
  glVertexAttribDivisor(4, 1);
  glVertexAttribDivisor(5, 1);
  glVertexAttribDivisor(6, 1);
  glVertexAttribDivisor(7, 1);
  glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, size,
                        reinterpret_cast<void *>(sizeof(float) * 0));
  glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, size,
                        reinterpret_cast<void *>(sizeof(float) * 4));
  glVertexAttribPointer(6, 4, GL_FLOAT, GL_FALSE, size,
                        reinterpret_cast<void *>(sizeof(float) * 8));
  glVertexAttribPointer(7, 4, GL_FLOAT, GL_FALSE, size,
                        reinterpret_cast<void *>(sizeof(float) * 12));
  auto ogl = gl_instancing(_instancing);
  ogl.vbo = vbo;

  if (_instancing.type == object_type::_2D) {
    m_instancing_2d.emplace_back(ogl);

  } else {
    m_instancing_3d.emplace_back(ogl);
  }
}

void gl_renderer::prepare() {
  if (!loadShader()) {
    std::cout << "failed to loads shader" << std::endl;
  }
}

void gl_renderer::registerTexture(handle_t handle) {
  ::SDL_Surface &surf = get_texture_system().get(handle);
  if (mTextureIDs.contains(handle)) {
    glDeleteTextures(1, &mTextureIDs[handle]);
    mTextureIDs.erase(handle);
  }
  ::SDL_LockSurface(&surf);
  auto formatbuf = ::SDL_AllocFormat(SDL_PIXELFORMAT_ABGR8888);
  formatbuf->BytesPerPixel = 4;
  auto imagedata = ::SDL_ConvertSurface(&surf, formatbuf, 0);
  SDL_UnlockSurface(&surf);
  // Generate a GL texture
  GLuint textureId;
  glGenTextures(1, &textureId);
  glBindTexture(GL_TEXTURE_2D, textureId);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surf.w, surf.h, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, imagedata->pixels);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  // Use linear filtering
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  SDL_FreeFormat(formatbuf);
  SDL_FreeSurface(imagedata);
  mTextureIDs[handle] = textureId;
}

bool gl_renderer::loadShader() {
  shader_parameter param{};
  if (!mSpriteShader.load("shader.vert", "shader.frag")) {
    return false;
  }
  mSpriteShader.create_ubo(0, sizeof(shader_parameter), &param);
  if (!mAlphaShader.load("shader.vert", "alpha.frag")) {
    return false;
  }
  mAlphaShader.create_ubo(0, sizeof(shader_parameter), &param);
  if (!mSpriteInstanceShader.load("shader_instance.vert", "shader.frag")) {
    return false;
  }
  mSpriteInstanceShader.create_ubo(0, sizeof(shader_parameter), &param);
  if (!mAlphaInstanceShader.load("shader_instance.vert", "alpha.frag")) {
    return false;
  }
  mAlphaInstanceShader.create_ubo(0, sizeof(shader_parameter), &param);
  return true;
}

} // namespace nen::gl
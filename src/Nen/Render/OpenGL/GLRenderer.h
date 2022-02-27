#pragma once
#include "manager/manager.hpp"
#include <cstdint>
#ifdef EMSCRIPTEN
#include <emscripten.h>
#include <emscripten/html5.h>
#endif
#if defined(EMSCRIPTEN) || defined(MOBILE)
#include <GLES3/gl3.h>
#endif
#include "ShaderGL.h"
#include <DrawObject/DrawObject.hpp>
#include <Nen.hpp>
#include <Render/Renderer.hpp>
#include <SDL.h>
#include <Texture/Texture.hpp>
#include <Vertex/VertexArray.hpp>
#include <memory>
#include <string>

namespace nen {
class renderer;

namespace gl {

struct VertexArrayForGL : public nen::vertex_array {
  /**
   * @brief Vertex Array Object
   *
   */
  uint32_t vao;

  /**
   * @brief Vertex Buffer Object
   *
   */
  uint32_t vbo;

  /**
   * @brief Index Buffer Object
   *
   */
  uint32_t ibo;
};

class ogl_instancing {
public:
  ogl_instancing(instancing &ins) : ins(ins) {}
  instancing &ins;
  uint32_t vbo;
};

class GLRenderer : public IRenderer {
public:
  GLRenderer(manager &_manager);
  ~GLRenderer() override {}

  void Initialize() override;
  void Render() override;
  void AddVertexArray(const vertex_array &vArray,
                      std::string_view name) override;
  void UpdateVertexArray(const vertex_array &vArray,
                         std::string_view name) override;
  void draw2d(std::shared_ptr<class draw_object> sprite) override;
  void draw3d(std::shared_ptr<class draw_object> sprite) override;

  void LoadShader(const shader &shaderInfo) override;
  void UnloadShader(const shader &shaderInfo) override;

  void add_instancing(instancing &_instancing) override;
  void remove_instancing(instancing &_instancing) override;

  void prepare();
  void cleanup() {}
  void registerTexture(handle_t handle);
  void pushSprite2d(std::shared_ptr<draw_object> sprite2d) {
    auto iter = mSprite2Ds.begin();
    for (; iter != mSprite2Ds.end(); ++iter) {
      if (sprite2d->drawOrder < (*iter)->drawOrder) {
        break;
      }
    }
    mSprite2Ds.insert(iter, sprite2d);
  }
  void eraseSprite2d(std::shared_ptr<draw_object> sprite2d) {
    auto itr = std::find(mSprite2Ds.begin(), mSprite2Ds.end(), sprite2d);
    if (itr != mSprite2Ds.end()) {
      mSprite2Ds.erase(itr);
    }
  }

  void pushSprite3d(std::shared_ptr<draw_object> sprite3d) {
    auto iter = mSprite3Ds.begin();
    for (; iter != mSprite3Ds.end(); ++iter) {
      if (sprite3d->drawOrder < (*iter)->drawOrder) {
        break;
      }
    }
    mSprite3Ds.insert(iter, sprite3d);
  }
  void eraseSprite3d(std::shared_ptr<draw_object> sprite3d) {
    auto itr = std::find(mSprite3Ds.begin(), mSprite3Ds.end(), sprite3d);
    if (itr != mSprite3Ds.end()) {
      mSprite3Ds.erase(itr);
    }
  }

private:
  bool loadShader();
  void draw_3d();
  void draw_2d();
  void draw_instancing_2d();
  void draw_instancing_3d();

  manager &m_manager;

  std::string vertexID;

  ShaderGL mSpriteShader;
  ShaderGL mAlphaShader;
  ShaderGL mSpriteInstanceShader;
  ShaderGL mAlphaInstanceShader;
  std::vector<std::pair<shader, ShaderGL>> userPipelines;
  GLuint mTextureID;
  std::unordered_map<handle_t, GLuint> mTextureIDs;
  std::unordered_map<std::string, VertexArrayForGL> m_VertexArrays;
  ::SDL_GLContext mContext;
  std::vector<std::shared_ptr<draw_object>> mSprite2Ds;
  std::vector<std::shared_ptr<draw_object>> mSprite3Ds;
  std::vector<ogl_instancing> m_instancing_2d;
  std::vector<ogl_instancing> m_instancing_3d;
  float prev_window_x;
  float prev_window_y;
};
} // namespace gl
} // namespace nen
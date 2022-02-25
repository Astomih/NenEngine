#pragma once
#include "../Color/Color.hpp"
#include "../Math/Math.hpp"
#include "../Math/Matrix4.hpp"
#include "../Math/Vector2.hpp"
#include "../Math/Vector3.hpp"
#include "../Shader/Shader.hpp"
#include "../Texture/Texture.hpp"
#include "../Texture/TextureType.hpp"
#include "../Vertex/VertexArray.hpp"
#include "../instancing/instancing.hpp"
#include "graphics_api.hpp"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace nen {

class renderer {
public:
  renderer(class manager &_manager);
  void initialize(graphics_api api);
  ~renderer() = default;

  graphics_api GetGraphicsAPI() { return RendererAPI; }

  void Shutdown();
  void UnloadData();

  void render();

  void draw2d(const std::shared_ptr<draw_object> draw_object);
  void draw3d(const std::shared_ptr<draw_object> draw_object);

  void AddVertexArray(const vertex_array &vArray, std::string_view name);
  void UpdateVertexArray(const vertex_array &vArray, std::string_view name);

  void add_instancing(instancing &_instancing);
  void remove_instancing(instancing &_instancing);

  void SetClearColor(const color &color) {
    if (color.r >= 0.f && color.g >= 0.f && color.b >= 0.f)
      clearColor = color;
  }

  color GetClearColor() { return this->clearColor; }

  void SetViewMatrix(const matrix4 &view) { mView = view; }
  const matrix4 &GetViewMatrix() { return mView; }
  void SetProjectionMatrix(const matrix4 &projection) {
    mProjection = projection;
  }
  const matrix4 &GetProjectionMatrix() { return mProjection; }

  class window &GetWindow();

  void toggleShowImGui() { showImGui = !showImGui; }
  bool isShowImGui() { return showImGui; }

  void LoadShader(const shader &shaderinfo);
  void UnloadShader(const shader &shaderinfo);

private:
  class manager &m_manager;
  void setup_shapes();
  color clearColor = palette::Black;

  // View/projection for 3D shaders
  matrix4 mView;
  matrix4 mProjection;

  // Window
  std::shared_ptr<class window> mWindow;
  // Renderer
  std::unique_ptr<class IRenderer> m_renderer;
  graphics_api RendererAPI;
  bool showImGui = false;
};

class IRenderer {
public:
  IRenderer() = default;
  virtual ~IRenderer() {}

  virtual void Initialize() {}
  virtual void Shutdown() {}
  virtual void Render() {}
  virtual void draw2d(std::shared_ptr<class draw_object> sprite) {}
  virtual void draw3d(std::shared_ptr<class draw_object> sprite) {}
  virtual void AddVertexArray(const vertex_array &vArray,
                              std::string_view name) {}
  virtual void UpdateVertexArray(const vertex_array &vArray,
                                 std::string_view name) {}

  virtual void add_instancing(instancing &_instancing) {}
  virtual void remove_instancing(instancing &_instancing) {}

  virtual void LoadShader(const shader &shaderInfo) {}
  virtual void UnloadShader(const shader &shaderInfo) {}
};
} // namespace nen
#ifndef SINEN_DRAW_OBJECT_HPP
#define SINEN_DRAW_OBJECT_HPP
#include "../math/vector2.hpp"
#include "../shader/shader.hpp"
#include "../texture/texture.hpp"
#include "../utility/handler.hpp"
#include "object_type.hpp"

namespace sinen {
class draw_object {
public:
  draw_object() = default;
  ~draw_object() = default;
  shader_parameter param;
  texture texture_handle;
  std::string vertexIndex;
  shader shader_data;
  bool is_draw_depth = true;
  int drawOrder = 100;
  int nodeNum = 1;
};

} // namespace nen
#endif // !SINEN_DRAW_OBJECT_HPP

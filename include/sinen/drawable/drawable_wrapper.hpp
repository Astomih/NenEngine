#ifndef SINEN_DRAW_OBJECT_WRAPPER_HPP
#define SINEN_DRAW_OBJECT_WRAPPER_HPP
#include "../math/vector2.hpp"
#include "../math/vector3.hpp"
#include "../texture/texture.hpp"
#include "drawable.hpp"

namespace sinen {
/**
 * @brief 2D draw object class
 *
 */
struct Draw2D {
public:
  /**
   * @brief Construct a new draw2d object
   *
   */
  Draw2D();
  /**
   * @brief Construct a new draw2d object
   * @param texture Texture to use
   *
   */
  Draw2D(Texture);
  /**
   * @brief Draw object
   *
   */
  void draw();
  /**
   * @brief Set user data
   *
   * @param index Index of user data 0-16
   * @param value Value of user data
   */
  void user_data_at(int index, float value);
  void add(const Vector2 &position, const float &rotation,
           const Vector2 &scale);
  void at(const int &index, const Vector2 &position, const float &rotation,
          const Vector2 &scale);
  void clear();
  struct world {
    Vector2 position;
    float rotation;
    Vector2 scale;
  };
  std::vector<world> worlds;
  Vector2 position;
  float rotation;
  Vector2 scale;
  Texture texture_handle;
  std::string vertex_name = "SPRITE";
  std::shared_ptr<Drawable> obj;
};
/**
 * @brief UI draw object class
 *
 */
struct DrawUI {
public:
  /**
   * @brief Construct a new drawui object
   *
   */
  DrawUI();
  /**
   * @brief Construct a new drawui object
   * @param texture Texture to use
   *
   */
  DrawUI(Texture);
  /**
   * @brief Draw object
   *
   */
  void draw();
  /**
   * @brief Set user data
   *
   * @param index Index of user data 0-16
   * @param value Value of user data
   */
  void user_data_at(int index, float value);
  void add(const Vector2 &position, const float &rotation,
           const Vector2 &scale);
  void at(const int &index, const Vector2 &position, const float &rotation,
          const Vector2 &scale);
  void clear();
  struct world {
    Vector2 position;
    float rotation;
    Vector2 scale;
  };
  std::vector<world> worlds;
  Vector2 position;
  float rotation;
  Vector2 scale;
  Texture texture_handle;
  std::string vertex_name = "SPRITE";
  std::shared_ptr<Drawable> obj;
};
/**
 * @brief 3D draw object class
 *
 */
class Draw3D {
public:
  /**
   * @brief Construct a new draw3d object
   *
   */
  Draw3D();
  /**
   * @brief Construct a new draw3d object
   * @param texture Texture to use
   *
   */
  Draw3D(Texture);
  /**
   * @brief Draw object
   *
   */
  void draw();
  /**
   * @brief Set user data
   *
   * @param index Index of user data 0-16
   * @param value Value of user data
   */
  void user_data_at(int index, float value);
  void add(const Vector3 &position, const Vector3 &rotation,
           const Vector3 &scale);
  void at(const int &index, const Vector3 &position, const Vector3 &rotation,
          const Vector3 &scale);
  void clear();
  Vector3 position;
  Vector3 rotation;
  Vector3 scale;
  Texture texture_handle;
  bool is_draw_depth = true;
  std::string vertex_name = "SPRITE";
  std::shared_ptr<Drawable> obj;
  struct world {
    Vector3 position;
    Vector3 rotation;
    Vector3 scale;
  };
  std::vector<world> worlds;
};
} // namespace sinen
#endif // !SINEN_DRAW_OBEJCT_WRAPPER_HPP

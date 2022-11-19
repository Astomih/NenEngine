#ifndef SINEN_EDITOR_HPP
#define SINEN_EDITOR_HPP
#include <memory>
#include <sinen/sinen.hpp>

namespace sinen {
/**
 * @brief Editor
 *
 */
class editor : public scene::implements {
public:
  /**
   * @brief Construct a new editor object
   *
   */
  editor();
  /**
   * @brief Destroy the editor object
   *
   */
  ~editor();
  /**
   * @brief Setup
   *
   */
  void setup() final override;
  /**
   * @brief Update the editor
   *
   * @param delta_time Delta time
   */
  void update(float delta_time) final override;

private:
  static void gizmo();
  static void inspector();
  static void menu();
  static camera::sub main_camera_clone;
  static std::vector<actor> m_actors;
  static std::vector<float *> m_matrices;
  static int index;
  class implements;
  std::unique_ptr<implements> m_impl;
};
} // namespace sinen
#endif

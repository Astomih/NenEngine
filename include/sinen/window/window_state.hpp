#ifndef SINEN_WINDOW_STATE_HPP
#define SINEN_WINDOW_STATE_HPP

namespace sinen {
enum class window_state {
  SHOWN = 1,
  HIDDEN,
  EXPOSED,

  MOVED,

  RESIZED,
  SIZE_CHANGED,

  MINIMIZED,
  MAXIMIZED,
  RESTORED,

  ENTER,
  LEAVE,
  FOCUS_GAINED,
  FOCUS_LOST,
  CLOSE,
  TAKE_FOCUS,
  HIT_TEST
};
}
#endif // !WINDOW_STATE_HPP

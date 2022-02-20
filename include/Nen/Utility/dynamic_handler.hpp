#ifndef NEN_DYNAMIC_HANDLER_HPP
#define NEN_DYNAMIC_HANDLER_HPP
#include <functional>
#include <iterator>
#include <memory>
#include <unordered_map>
#include <utility>

#include "Utility/handle_t.hpp"
#include "handle_t.hpp"

namespace nen {
template <class T> class dynamic_handler {
public:
  dynamic_handler() = default;
  dynamic_handler(const dynamic_handler &) = delete;
  dynamic_handler(dynamic_handler &&) = default;
  std::unordered_map<handle_t, std::unique_ptr<T>> data;

  T &operator[](const handle_t &handle) {
    return *reinterpret_cast<T *>(data[handle].get());
  }
  template <class S = T, typename... _Args>
  S &add(handle_t &handle, _Args &&...__args) {
    handle = 0;
    while (data.contains(handle))
      ++handle;
    data.emplace(handle, std::make_unique<S>(std::forward<_Args>(__args)...));
    return get<S>(handle);
  }
  template <class S = T> S &get(const handle_t &handle) {
    return *reinterpret_cast<S *>(data[handle].get());
  }
  void clear() { data.clear(); }
  bool contains(const handle_t &handle) { return data.contains(handle); }
  void remove(const handle_t &handle) { data.erase(handle); }
};
} // namespace nen
#endif
#include <Nen.hpp>
namespace nen {
void vertex_array::PushIndices(uint32_t indexArray[], size_t count) {
  for (size_t i = 0; i < count; i++) {
    indices.push_back(indexArray[i]);
  }
}
} // namespace nen
#include <Nen.hpp>

namespace nen {
bool collision::IntersectAABB(const vector3 &a, const vector3 &b,
                              const vector3 &detectSpace) {
  if (!(a.x - detectSpace.x <= b.x && b.x <= a.x + detectSpace.x))
    return false;
  if (!(a.y - detectSpace.y <= b.y && b.y <= a.y + detectSpace.y))
    return false;
  if (!(a.z - detectSpace.z <= b.z && b.z <= a.z + detectSpace.z))
    return false;
  return true;
}
} // namespace nen

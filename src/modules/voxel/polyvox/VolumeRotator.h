/**
 * @file
 */

#pragma once

#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include "math/Axis.h"

namespace voxel {

class RawVolume;
class Voxel;

extern RawVolume* rotateVolume(const RawVolume* source, const glm::vec3& angles, const Voxel& empty, const glm::vec3& pivot, bool increaseSize = true);
extern RawVolume* rotateAxis(const RawVolume* source, math::Axis axis);

}

/**
 * @file
 */

#pragma once

#include "voxel/RawVolume.h"

namespace voxedit {
namespace tool {

extern voxel::RawVolume* resize(const voxel::RawVolume* source, const glm::ivec3& size);

}
}

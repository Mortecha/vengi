/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include <glm/vec2.hpp>
#include <glm/vec4.hpp>

namespace voxelformat {

struct ThumbnailContext {
	glm::ivec2 outputSize{128, 128};
	glm::vec4 clearColor{0.0f, 0.0f, 0.0f, 1.0f};
	float pitch = 0.0f;
	float yaw = 0.0f;
	float roll = 0.0f;
	double deltaFrameSeconds = 0.001;
};

class SceneGraph;
class SceneGraphNode;

/**
 * @brief Callback to create a thumbnail for saving the current scene graph.
 * Some formats supports storing embedded screenshots of the voxel model. This callback
 * must return a RGBA image in the given size
 * @note duplicated in VolumeFormat.h
 */
typedef image::ImagePtr (*ThumbnailCreator)(const SceneGraph &, const ThumbnailContext &);

} // namespace voxelformat

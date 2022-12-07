/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "voxelformat/FormatThumbnail.h"
#include "io/Stream.h"

namespace voxelformat {
class SceneGraph;
}

namespace voxelrender {

image::ImagePtr volumeThumbnail(const core::String &fileName, io::SeekableReadStream &stream, const voxelformat::ThumbnailContext &ctx);
image::ImagePtr volumeThumbnail(const voxelformat::SceneGraph &sceneGraph, const voxelformat::ThumbnailContext &ctx);

} // namespace voxelrender

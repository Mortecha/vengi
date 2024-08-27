/**
 * @file
 */

#pragma once

#include "image/Image.h"
#include "voxel/Face.h"

namespace voxel {
class RawVolumeWrapper;
class RawVolume;
class Voxel;
}

namespace palette {
class PaletteLookup;
class Palette;
}

namespace voxelutil {

/**
 * @brief Import a heightmap with rgb being the surface color and alpha channel being the height
 */
void importColoredHeightmap(voxel::RawVolumeWrapper& volume, palette::PaletteLookup &palLookup, const image::ImagePtr& image, const voxel::Voxel &underground);
void importHeightmap(voxel::RawVolumeWrapper& volume, const image::ImagePtr& image, const voxel::Voxel &underground, const voxel::Voxel &surface);
int importHeightMaxHeight(const image::ImagePtr &image, bool alpha);
voxel::RawVolume* importAsPlane(const image::ImagePtr& image, const palette::Palette &palette, uint8_t thickness = 1);
voxel::RawVolume* importAsPlane(const image::ImagePtr& image, uint8_t thickness = 1);
voxel::RawVolume* importAsPlane(const image::Image *image, const palette::Palette &palette, uint8_t thickness = 1);
voxel::RawVolume* importAsPlane(const image::Image *image, uint8_t thickness = 1);
core::String getDefaultDepthMapFile(const core::String &imageName, const core::String &postfix = "-dm");
voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
voxel::RawVolume* importAsVolume(const image::ImagePtr& image, const image::ImagePtr& depthMap, const palette::Palette &palette, uint8_t maxDepth, bool bothSides = false);
voxel::RawVolume* importAsVolume(const image::ImagePtr& image, uint8_t maxDepth, bool bothSides = false);
bool importFace(voxel::RawVolume &volume, const palette::Palette &palette, voxel::FaceNames faceName, const image::ImagePtr &image, const glm::vec2 &uv0, const glm::vec2 &uv1, image::TextureWrap wrapS = image::TextureWrap::Repeat, image::TextureWrap wrapT = image::TextureWrap::Repeat, uint8_t replacementPalIdx = 0);

}

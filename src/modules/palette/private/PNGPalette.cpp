/**
 * @file
 */

#include "PNGPalette.h"
#include "core/Log.h"
#include "image/Image.h"
#include "palette/Palette.h"

namespace palette {

bool PNGPalette::load(const core::String &filename, io::SeekableReadStream &stream, palette::Palette &palette) {
	image::ImagePtr img = image::createEmptyImage(filename);
	img->load(stream, stream.size());
	return palette.load(img);
}

bool PNGPalette::save(const palette::Palette &palette, const core::String &filename, io::SeekableWriteStream &stream) {
	image::Image img(filename);
	core::RGBA colors[PaletteMaxColors];
	for (int i = 0; i < PaletteMaxColors; i++) {
		colors[i] = palette.color(i);
	}
	// must be palette::PaletteMaxColors - otherwise the exporter uv coordinates must get adopted
	img.loadRGBA((const uint8_t *)colors, PaletteMaxColors, 1);
	if (!img.writePng(stream)) {
		Log::warn("Failed to write the palette file '%s'", filename.c_str());
		return false;
	}
	return true;
}

} // namespace voxel

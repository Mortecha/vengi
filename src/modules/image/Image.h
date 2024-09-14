/**
 * @file
 */

#pragma once

#include "core/NonCopyable.h"
#include "core/RGBA.h"
#include "io/IOResource.h"
#include "io/File.h"
#include "core/SharedPtr.h"
#include "io/Stream.h"
#include <glm/fwd.hpp>
#include <glm/vec2.hpp>

namespace image {

// https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/glTexParameter.xhtml
enum TextureWrap : uint8_t {
	Repeat, // causes the integer part of the s coordinate to be ignored; the GL uses only the fractional part, thereby
			// creating a repeating pattern.
	ClampToEdge, // causes s coordinates to be clamped to the range [1/2N,1−1/2N], where N is the size of the texture in
				 // the direction of clamping.
	MirroredRepeat, // causes the s coordinate to be set to the fractional part of the texture coordinate if the integer
					// part of s is even; if the integer part of s is odd, then the s texture coordinate is set to
					// 1−frac(s), where frac(s) represents the fractional part of s

	Max
};

/**
 * @brief Wrapper for image loading
 */
class Image: public io::IOResource, core::NonCopyable {
private:
	core::String _name;
	int _width = -1;
	int _height = -1;
	int _depthOfColor = -1;
	uint8_t* _data = nullptr;

public:
	Image(const core::String& name);
	~Image();

	template<typename FUNC>
	bool load(int w, int h, FUNC &&func) {
		_depthOfColor = 4;
		if (!resize(w, h)) {
			_state = io::IOSTATE_FAILED;
			return false;
		}
		for (int y = 0; y < h; ++y) {
			for (int x = 0; x < w; ++x) {
				core::RGBA rgba;
				func(x, y, rgba);
				setColor(rgba, x, y);
			}
		}
		_state = io::IOSTATE_LOADED;
		return true;
	}
	bool load(const io::FilePtr& file);
	bool load(const uint8_t* buffer, int length);
	bool load(io::ReadStream &stream, int length);
	/**
	 * Loads a raw RGBA buffer
	 */
	bool loadRGBA(const uint8_t* buffer, int width, int height);
	bool loadRGBA(io::ReadStream& stream, int w, int h);
	bool loadBGRA(io::ReadStream& stream, int w, int h);

	static glm::ivec2 pixels(const glm::vec2 &uv, int w, int h, TextureWrap wrapS = TextureWrap::Repeat, TextureWrap wrapT = TextureWrap::Repeat, bool originUpperLeft = false);
	glm::ivec2 pixels(const glm::vec2 &uv, TextureWrap wrapS = TextureWrap::Repeat, TextureWrap wrapT = TextureWrap::Repeat, bool originUpperLeft = false) const;
	/**
	 * @sa MeshFormat::paletteUV()
	 */
	glm::vec2 uv(int x, int y, bool originUpperLeft = false) const;
	static glm::vec2 uv(int x, int y, int w, int h, bool originUpperLeft = false);

	bool resize(int w, int h);

	static void flipVerticalRGBA(uint8_t *pixels, int w, int h);
	bool writePng(io::SeekableWriteStream &stream) const;
	static bool writePng(io::SeekableWriteStream &stream, const uint8_t* buffer, int width, int height, int depth);
	/**
	 * @param[in] quality Ranges from 1 to 100 where higher is better
	 */
	static bool writeJPEG(io::SeekableWriteStream &stream, const uint8_t* buffer, int width, int height, int depth, int quality = 100);
	bool writeJPEG(io::SeekableWriteStream &stream, int quality = 100) const;
	core::String pngBase64() const;
	core::RGBA colorAt(int x, int y) const;
	core::RGBA colorAt(const glm::vec2 &uv, TextureWrap wrapS = TextureWrap::Repeat,
					   TextureWrap wrapT = TextureWrap::Repeat, bool originUpperLeft = false) const;

	bool isGrayScale() const;

	void setColor(core::RGBA rgba, int x, int y);

	const uint8_t* at(int x, int y) const;

	void setName(const core::String &name) {
		_name = name;
	}

	inline const core::String& name() const {
		return _name;
	}

	inline const uint8_t* data() const {
		return _data;
	}

	inline glm::vec2 size() const {
		return {_width, _height};
	}

	inline int width() const {
		return _width;
	}

	inline int height() const {
		return _height;
	}

	inline int depth() const {
		return _depthOfColor;
	}

	inline float aspect() const {
		return (float)_width / (float)_height;
	}
};

typedef core::SharedPtr<Image> ImagePtr;

// creates an empty image
inline ImagePtr createEmptyImage(const core::String& name) {
	return core::make_shared<Image>(name);
}

uint8_t* createPng(const void *pixels, int width, int height, int depth, int *pngSize);
ImagePtr loadImage(const io::FilePtr& file);
ImagePtr loadImage(const core::String &name, io::SeekableReadStream &stream, int length = -1);
ImagePtr loadImage(const core::String &name, io::ReadStream &stream, int length);
ImagePtr loadRGBAImageFromStream(const core::String &name, io::ReadStream &stream, int w, int h);

/**
 * @brief If there is no extension given, all supported extensions are tried
 */
ImagePtr loadImage(const core::String& filename);

bool writeImage(const image::Image &image, io::SeekableWriteStream& stream);
bool writeImage(const image::ImagePtr &image, io::SeekableWriteStream& stream);
bool writeImage(const image::Image &image, const core::String& filename);
bool writeImage(const image::ImagePtr &image, const core::String& filename);
core::String print(const image::ImagePtr &image, bool limited = true);

}

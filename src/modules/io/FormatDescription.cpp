/**
 * @file
 */

#include "FormatDescription.h"
#include "core/Algorithm.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/StringUtil.h"

namespace io {

namespace format {

FormatDescription png() {
	return {"Portable Network Graphics", {"png"}, {}, FORMAT_FLAG_SAVE};
}

const FormatDescription *images() {
	// clang-format: off
	static thread_local FormatDescription desc[] = {
		png(),
		{"JPEG", {"jpeg", "jpg"}, {}, FORMAT_FLAG_SAVE},
		{"Targa image file", {"tga"}, {}, 0u},
		{"DDS", {"dds"}, {}, 0u},
		{"PKM", {"pkm"}, {}, 0u},
		{"PVR", {"pvr"}, {}, 0u},
		{"Bitmap", {"bmp"}, {}, 0u},
		{"Photoshop", {"psd"}, {}, 0u},
		{"Graphics Interchange Format", {"gif"}, {}, 0u},
		{"Radiance rgbE", {"hdr"}, {}, 0u},
		{"Softimage PIC", {"pic"}, {}, 0u},
		{"Portable Anymap", {"pnm"}, {}, 0u},
		{"", {}, {}, 0u}
	};
	// clang-format: on
	return desc;
}

const FormatDescription *fonts() {
	// clang-format: off
	static thread_local FormatDescription desc[] = {
		{"TrueType Font", {"ttf"}, {}, 0u},
		{"", {}, {}, 0u}
	};
	// clang-format: on
	return desc;
}

FormatDescription jascPalette() {
	return {"JASC Palette", {"pal"}, {"JASC"}, FORMAT_FLAG_SAVE};
}

FormatDescription gimpPalette() {
	return {"Gimp Palette", {"gpl"}, {}, FORMAT_FLAG_SAVE};
}

const FormatDescription *palettes() {
	// clang-format: off
	static thread_local FormatDescription desc[] = {
		gimpPalette(),
		{"Qubicle Palette", {"qsm"}, {}, 0u},
		jascPalette(),
		{"Photoshop Palette", {"ase"}, {"ASEF"}, FORMAT_FLAG_SAVE},
		{"RGB Palette", {"pal"}, {}, FORMAT_FLAG_SAVE},
		{"CSV Palette", {"csv"}, {}, FORMAT_FLAG_SAVE},
		{"Portable Network Graphics", {"png"}, {"\x89PNG"}, FORMAT_FLAG_SAVE},
		{"", {}, {}, 0u}
	};
	// clang-format: on
	return desc;
}

const FormatDescription *lua() {
	// clang-format: off
	static thread_local FormatDescription desc[] = {
		{"LUA script", {"lua"}, {}, 0u},
		{"", {}, {}, 0u}
	};
	// clang-format: on
	return desc;
}

} // namespace format

core::String FormatDescription::mainExtension(bool includeDot) const {
	if (exts.empty()) {
		return "";
	}
	if (!includeDot) {
		return exts[0];
	}
	return "." + exts[0];
}

bool FormatDescription::matchesExtension(const core::String &fileExt) const {
	const core::String &lowerExt = fileExt.toLower();
	for (const core::String &ext : exts) {
		if (lowerExt == ext) {
			return true;
		}
	}
	return false;
}

bool FormatDescription::operator<(const FormatDescription &rhs) const {
	return name < rhs.name;
}

bool FormatDescription::operator>(const FormatDescription &rhs) const {
	return name > rhs.name;
}

core::String FormatDescription::wildCard() const {
	core::String pattern;
	for (size_t i = 0; i < exts.size(); ++i) {
		if (i > 0) {
			pattern.append(",");
		}
		pattern += core::string::format("*.%s", exts[i].c_str());
	}
	return pattern;
}

void FileDescription::set(const core::String &s, const io::FormatDescription *f) {
	if (f != nullptr) {
		desc = *f;
	} else {
		desc = {};
	}
	name = s;
}

void FileDescription::clear() {
	name = "";
	desc = {};
}

bool isA(const core::String &file, const io::FormatDescription *desc) {
	const core::String &ext = core::string::extractExtension(file);
	for (; desc->valid(); ++desc) {
		if (desc->matchesExtension(ext)) {
			return true;
		}
	}
	return false;
}

bool isImage(const core::String &file) {
	return isA(file, io::format::images());
}

void createGroupPatterns(const FormatDescription *inputDesc, core::DynamicArray<io::FormatDescription> &groups) {
	core::DynamicArray<io::FormatDescription> descs;
	for (; inputDesc->valid(); ++inputDesc) {
		descs.emplace_back(*inputDesc);
	}
	core::sort(descs.begin(), descs.end(), core::Less<io::FormatDescription>());
	core::DynamicArray<io::FormatDescription> temp;
	core::String lastName;
	for (const io::FormatDescription &desc : descs) {
		core::String firstWord = desc.name;
		auto iter = firstWord.find_first_of(" ");
		if (iter != core::String::npos) {
			firstWord = firstWord.substr(0, iter);
		}
		if (lastName != firstWord) {
			if (temp.size() >= 2) {
				io::FormatDescriptionExtensions exts{};
				uint32_t flags = FORMAT_FLAG_GROUP;
				for (const io::FormatDescription &tmpDesc : temp) {
					for (const core::String &ext : tmpDesc.exts) {
						exts.push_back(ext);
					}
					flags |= tmpDesc.flags;
				}
				const io::FormatDescription val{lastName, exts, {}, flags};
				groups.push_back(val);
			}
			lastName = firstWord;
			temp.clear();
		}
		temp.push_back(desc);
	}
	if (temp.size() >= 2) {
		io::FormatDescriptionExtensions exts{};
		uint32_t flags = FORMAT_FLAG_GROUP;
		for (const io::FormatDescription &tmpDesc : temp) {
			for (const core::String &ext : tmpDesc.exts) {
				exts.push_back(ext);
			}
			flags |= tmpDesc.flags;
		}
		const io::FormatDescription val{lastName, exts, {}, flags};
		groups.push_back(val);
	}
}

core::String convertToFilePattern(const FormatDescription &desc) {
	core::String pattern;
	bool showName = false;
	if (!desc.name.empty()) {
		pattern += desc.name;
		if (!desc.exts.empty()) {
			pattern.append(" (");
			showName = true;
		}
	}
	pattern += desc.wildCard();
	if (showName) {
		pattern.append(")");
	}
	return pattern;
}

core::String convertToAllFilePattern(const FormatDescription *desc) {
	core::String pattern;
	int j = 0;
	while (desc->valid()) {
		if (j > 0) {
			pattern.append(",");
		}
		pattern += desc->wildCard();
		++desc;
		++j;
	}
	return j > 1 ? pattern : "";
}

uint32_t loadMagic(io::SeekableReadStream &stream) {
	uint32_t magicWord = 0u;
	stream.peekUInt32(magicWord);
	return magicWord;
}

bool isA(const io::FormatDescription &desc, uint32_t magic) {
	for (const core::String &m : desc.magics) {
		const size_t l = m.size();
		const char f1 = l > 0 ? m[0] : '\0';
		const char f2 = l > 1 ? m[1] : '\0';
		const char f3 = l > 2 ? m[2] : '\0';
		const char f4 = l > 3 ? m[3] : '\0';
		if (FourCC(f1, f2, f3, f4) == magic) {
			return true;
		}
	}
	return false;
}

const io::FormatDescription *getDescription(const core::String &filename, uint32_t magic,
											const io::FormatDescription *descriptions) {
	const core::String ext = core::string::extractExtension(filename);
	const core::String extFull = core::string::extractAllExtensions(filename);
	for (const io::FormatDescription *desc = descriptions; desc->valid(); ++desc) {
		if (!desc->matchesExtension(ext) && !desc->matchesExtension(extFull)) {
			continue;
		}
		if (magic > 0 && !desc->magics.empty() && !isA(*desc, magic)) {
			Log::debug("File doesn't have the expected magic number");
			continue;
		}
		return desc;
	}
	if (magic > 0) {
		// search again - but this time only the magic bytes...
		for (const io::FormatDescription *desc = descriptions; desc->valid(); ++desc) {
			if (desc->magics.empty()) {
				continue;
			}
			if (!isA(*desc, magic)) {
				continue;
			}
			return desc;
		}
	}
	if (extFull.empty()) {
		Log::debug("Could not identify the format");
	} else {
		Log::debug("Could not find a supported format description for '%s' ('%s')", extFull.c_str(), filename.c_str());
	}
	return nullptr;
}

const io::FormatDescription *getDescription(const io::FileDescription &fileDesc, uint32_t magic,
											const io::FormatDescription *descriptions) {
	if (fileDesc.desc.valid()) {
		return &fileDesc.desc;
	}
	const core::String filename = fileDesc.name;
	return io::getDescription(filename, magic, descriptions);
}

} // namespace io

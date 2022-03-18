/**
 * @file
 */

#include "VXRFormat.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/Common.h"
#include "core/FourCC.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "app/App.h"
#include "VXMFormat.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/FileStream.h"
#include "io/Filesystem.h"
#include "io/Stream.h"
#include "voxel/RawVolume.h"
#include "voxelformat/SceneGraph.h"
#include "voxelformat/SceneGraphNode.h"
#include "voxelformat/VXAFormat.h"
#include <glm/common.hpp>
#include <glm/gtc/quaternion.hpp>

namespace voxelformat {

#define wrap(read) \
	if ((read) != 0) { \
		Log::error("Could not load vmr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

#define wrapBool(read) \
	if ((read) != true) { \
		Log::error("Could not load vmr file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)", (int)__LINE__); \
		return false; \
	}

bool VXRFormat::saveRecursiveNode(const core::String &name, const SceneGraphNode& node, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeString(node.name(), true))
	const core::String baseName = core::string::stripExtension(filename);
	const core::String finalName = baseName + name + ".vxm";
	wrapBool(stream.writeString(finalName, true))
	VXMFormat f;
	io::FilePtr outputFile = io::filesystem()->open(finalName, io::FileMode::SysWrite);
	if (!outputFile) {
		Log::error("Failed to open %s for writing", finalName.c_str());
		return false;
	}
	io::FileStream wstream(outputFile);
	SceneGraph newSceneGraph;
	SceneGraphNode newNode;
	newNode.setVolume(node.volume(), false);
	newNode.setName(name);
	newNode.setVisible(node.visible());
	newSceneGraph.emplace(core::move(newNode));
	wrapBool(f.saveGroups(newSceneGraph, finalName, wstream))

	wrapBool(stream.writeInt32(0)); // next child count
#if 0
	for (int i = 0; i < childCount; ++i) {
		wrapBool(saveRecursiveNode(i, volumes, filename, stream))
	}
#endif
	return true;
}

bool VXRFormat::saveGroups(const SceneGraph& sceneGraph, const core::String &filename, io::SeekableWriteStream& stream) {
	wrapBool(stream.writeUInt32(FourCC('V','X','R','4')));

	const int childCount = (int)sceneGraph.size();
	wrapBool(stream.writeInt32(childCount))
	int n = 0;
	for (const SceneGraphNode& node : sceneGraph) {
		core::String name = node.name();
		if (name.empty()) {
			name = core::string::format("%i", n);
		}
		wrapBool(saveRecursiveNode(name, node, filename, stream))
		++n;
	}
	return true;
}

bool VXRFormat::loadChildVXM(const core::String& vxmPath, SceneGraphNode &node, int version) {
	const io::FilePtr& file = io::filesystem()->open(vxmPath);
	if (!file->validHandle()) {
		Log::error("Could not open file '%s'", vxmPath.c_str());
		return false;
	}
	io::FileStream stream(file);
	VXMFormat f;
	SceneGraph childSceneGraph;
	if (!f.loadGroups(vxmPath, stream, childSceneGraph)) {
		Log::error("Failed to load '%s'", vxmPath.c_str());
		return false;
	}
	const int modelCount = (int)childSceneGraph.size(SceneGraphNodeType::Model);
	if (modelCount > 1) {
		Log::warn("Unexpected scene graph found in vxm file - only use the first one");
	} else if (modelCount != 1) {
		Log::error("No models found in vxm file: %i", modelCount);
		return false;
	}
	SceneGraphNode* childModelNode = childSceneGraph[0];
	core_assert_always(childModelNode != nullptr);
	childModelNode->releaseOwnership();
	node.setVolume(childModelNode->volume(), true);
	node.setVisible(childModelNode->visible());
	node.setLocked(childModelNode->locked());
	node.addProperties(childModelNode->properties());
	// TODO: this shouldn't be needed - should get set in vxa or vxr (or in vxm version <= 3)
	const voxelformat::SceneGraphTransform &childTransform = childModelNode->transform(0);
	voxelformat::SceneGraphTransform &transform = node.transform(0);
	transform.setPivot(childTransform.pivot());
	return true;
}

bool VXRFormat::importChildVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent) {
	SceneGraphNode node(SceneGraphNodeType::Model);
	char nodeId[1024];
	wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
	node.setName(nodeId);
	node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
	uint32_t animCnt;
	wrap(stream.readUInt32(animCnt))
	char animationId[1024];
	wrapBool(stream.readString(sizeof(animationId), animationId, true))
	node.setProperty("animationid", animationId);
	sceneGraph.addAnimation(animationId);
	int32_t keyFrameCount;
	wrap(stream.readInt32(keyFrameCount))
	for (int32_t i = 0u; i < keyFrameCount; ++i) {
		SceneGraphKeyFrame& keyFrame = node.keyFrame(i);
		wrap(stream.readUInt32(keyFrame.frame)) // frame index
		int32_t interpolation;
		// instant = 0
		// linear = 1
		// quad ease in = 2
		// quad ease out = 3
		// quad ease in out = 4
		// cubic ease in = 5
		// cubic ease out = 6
		// cubic ease in out = 7
		wrap(stream.readInt32(interpolation))
		if (version > 1) {
			stream.readBool(); // rotation ??
		}
		keyFrame.transform.setPivot(glm::vec3(0.5f));
		glm::vec3 localPosition{0.0f};
		glm::quat localRot{0.0f, 0.0f, 0.0f, 0.0f};
		float localScale = 1.0f;
		glm::vec3 translation;
		wrap(stream.readFloat(translation.x))
		//nodeFrame.transform.position.x *= -1.0f;
		wrap(stream.readFloat(translation.y))
		wrap(stream.readFloat(translation.z))
		keyFrame.transform.setTranslation(translation);
		if (version >= 3) {
			wrap(stream.readFloat(localPosition.x))
			wrap(stream.readFloat(localPosition.y))
			wrap(stream.readFloat(localPosition.z))
		}
		if (version == 1) {
			float rotationx;
			float rotationy;
			float rotationz;
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			keyFrame.transform.setOrientation(glm::quat(glm::vec3(rotationx, rotationy, rotationz)));
			wrap(stream.readFloat(rotationx))
			wrap(stream.readFloat(rotationy))
			wrap(stream.readFloat(rotationz))
			localRot = glm::quat(glm::vec3(rotationx, rotationy, rotationz));
		} else {
			glm::quat orientation;
			wrap(stream.readFloat(orientation.x))
			wrap(stream.readFloat(orientation.y))
			wrap(stream.readFloat(orientation.z))
			wrap(stream.readFloat(orientation.w))
			keyFrame.transform.setOrientation(orientation);
			wrap(stream.readFloat(localRot.x))
			wrap(stream.readFloat(localRot.y))
			wrap(stream.readFloat(localRot.z))
			wrap(stream.readFloat(localRot.w))
		}
		float scale;
		wrap(stream.readFloat(scale))
		keyFrame.transform.setScale(scale);
		if (version >= 3) {
			wrap(stream.readFloat(localScale))
		}
		keyFrame.transform.update();
	}
	int32_t children;
	wrap(stream.readInt32(children))
	const int modelNode = sceneGraph.emplace(core::move(node), parent);
	for (int32_t i = 0u; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, modelNode))
	}
	return true;
}

// the positions that were part of the previous vxr versions are now in vxa
bool VXRFormat::importChild(const core::String& vxmPath, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version, int parent) {
	SceneGraphNode node(SceneGraphNodeType::Model);
	char id[1024];
	wrapBool(stream.readString(sizeof(id), id, true))
	char filename[1024];
	wrapBool(stream.readString(sizeof(filename), filename, true))
	if (filename[0] != '\0') {
		const core::String modelPath = core::string::path(core::string::extractPath(vxmPath), filename);
		if (!loadChildVXM(modelPath, node, version)) {
			Log::warn("Failed to attach model for id '%s' with filename %s (%s)", id, filename, modelPath.c_str());
		}
	}
	if (node.volume() == nullptr) {
		node = SceneGraphNode(SceneGraphNodeType::Group);
	}
	node.setName(id);
	node.setProperty("id", id);
	node.setProperty("filename", filename);
	if (version > 4) {
		if (version >= 9) {
			node.setProperty("collidable", stream.readBool());
			node.setProperty("decorative", stream.readBool());
		}
		if (version >= 6) {
			uint32_t color;
			wrap(stream.readUInt32(color))
			node.setProperty("color", core::Color::toHex(color));
			node.setProperty("favorite", stream.readBool());
			node.setProperty("visible", stream.readBool());
		}
		node.setProperty("mirror x axis", stream.readBool());
		node.setProperty("mirror y axis", stream.readBool());
		node.setProperty("mirror z axis", stream.readBool());
		node.setProperty("preview mirror x axis", stream.readBool());
		node.setProperty("preview mirror y axis", stream.readBool());
		node.setProperty("preview mirror z axis", stream.readBool());
		node.setProperty("ikAnchor", stream.readBool());
		if (version >= 9) {
			char effectorId[1024];
			wrapBool(stream.readString(sizeof(effectorId), effectorId, true))
			node.setProperty("effectorId", effectorId);
			node.setProperty("constraints visible", stream.readBool());
			float rollmin;
			wrap(stream.readFloat(rollmin))
			node.setProperty("rollmin", core::string::format("%f", rollmin));
			float rollmax;
			wrap(stream.readFloat(rollmax))
			node.setProperty("rollmax", core::string::format("%f", rollmax));
			int32_t inverseKinematicsConstraints;
			wrap(stream.readInt32(inverseKinematicsConstraints))
			for (int32_t i = 0; i < inverseKinematicsConstraints; ++i) {
				float inverseKinematicsX, inverseKinematicsY, inverseKinematicsRadius;
				wrap(stream.readFloat(inverseKinematicsX))
				wrap(stream.readFloat(inverseKinematicsY))
				wrap(stream.readFloat(inverseKinematicsRadius))
			}
		} else {
			float dummyf;
			stream.readBool(); // ???
			wrap(stream.readFloat(dummyf)) // ???
			wrap(stream.readFloat(dummyf)) // ???
			stream.readBool(); // ???
			stream.readBool(); // ???
			stream.readBool(); // ???
			stream.readBool(); // ???
		}
	}
	const int nodeId = sceneGraph.emplace(core::move(node), parent);
	if (version >= 4) {
		int32_t children = 0;
		wrap(stream.readInt32(children))
		for (int32_t i = 0; i < children; ++i) {
			wrapBool(importChild(vxmPath, stream, sceneGraph, version, nodeId != -1 ? nodeId : parent))
		}
	}
	return true;
}

image::ImagePtr VXRFormat::loadScreenshot(const core::String &filename, io::SeekableReadStream& stream) {
	const core::String imageName = filename + ".png";
	return image::loadImage(imageName, false);
}

bool VXRFormat::loadGroupsVersion3AndEarlier(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version) {
	uint32_t childAndModelCount;
	wrap(stream.readUInt32(childAndModelCount))
	uint32_t children = 0;
	wrap(stream.readUInt32(children))
	const int rootNodeId = sceneGraph.root().id();
	SceneGraphNode &rootNode = sceneGraph.node(rootNodeId);
	for (uint32_t i = 0; i < children; ++i) {
		wrapBool(importChildVersion3AndEarlier(filename, stream, sceneGraph, version, rootNodeId))
	}
	int32_t modelCount;
	wrap(stream.readInt32(modelCount))
	for (int32_t i = 0; i < modelCount; ++i) {
		char nodeId[1024];
		wrapBool(stream.readString(sizeof(nodeId), nodeId, true))
		SceneGraphNode* node = sceneGraph.findNodeByName(nodeId);
		if (node == nullptr || node->type() != SceneGraphNodeType::Model) {
			Log::error("Can't find referenced model node %s", nodeId);
			return false;
		}
		char vxmFilename[1024];
		wrapBool(stream.readString(sizeof(vxmFilename), vxmFilename, true))
		if (vxmFilename[0] != '\0') {
			const core::String modelPath = core::string::path(core::string::extractPath(filename), vxmFilename);
			if (!loadChildVXM(modelPath, *node, version)) {
				Log::warn("Failed to attach model for %s with filename %s", nodeId, modelPath.c_str());
			}
		}
	}
	const uint8_t frameIdx = core::Var::getSafe(cfg::VoxformatFrame)->intVal();
	recursiveTransformVolume(sceneGraph, rootNode, rootNode.transform(frameIdx), frameIdx);

	return true;
}

bool VXRFormat::handleVersion8AndLater(io::SeekableReadStream& stream, SceneGraphNode &node) {
	char baseTemplate[1024];
	wrapBool(stream.readString(sizeof(baseTemplate), baseTemplate, true))
	node.setProperty("basetemplate", baseTemplate);
	const bool isStatic = stream.readBool();
	node.setProperty("static", isStatic);
	if (isStatic) {
		int32_t lodLevels;
		wrap(stream.readInt32(lodLevels))
		for (int32_t i = 0 ; i < lodLevels; ++i) {
			uint32_t dummy;
			wrap(stream.readUInt32(dummy))
			wrap(stream.readUInt32(dummy))
			uint32_t diffuseTexZipped;
			wrap(stream.readUInt32(diffuseTexZipped))
			stream.skip(diffuseTexZipped);
			const bool hasEmissive = stream.readBool();
			if (hasEmissive) {
				uint32_t emissiveTexZipped;
				wrap(stream.readUInt32(emissiveTexZipped))
				stream.skip(emissiveTexZipped);
			}
			int32_t quadAmount;
			wrap(stream.readInt32(quadAmount))
			for (int32_t quad = 0; quad < quadAmount; ++quad) {
				for (int v = 0; v < 4; ++v) {
					float dummyFloat;
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
					wrap(stream.readFloat(dummyFloat))
				}
			}
		}
	}
	return true;
}

bool VXRFormat::loadGroupsVersion4AndLater(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph, int version) {
	const int rootNodeId = sceneGraph.root().id();
	SceneGraphNode &rootNode = sceneGraph.node(rootNodeId);

	// TODO: allow to specify the animation to import... (via cvar?)
	char defaultAnim[1024] = "";
	if (version >= 7) {
		wrapBool(stream.readString(sizeof(defaultAnim), defaultAnim, true))
		rootNode.setProperty("defaultanim", defaultAnim);
	}

	int32_t children = 0;
	wrap(stream.readInt32(children))

	if (version >= 8) {
		handleVersion8AndLater(stream, rootNode);
	}

	Log::debug("Found %i children", children);
	for (int32_t i = 0; i < children; ++i) {
		wrapBool(importChild(filename, stream, sceneGraph, version, rootNodeId))
	}

	const core::String& basePath = core::string::extractPath(filename);
	core::DynamicArray<io::Filesystem::DirEntry> entities;
	io::filesystem()->list(basePath, entities, "*.vxa");
	core::String vxaPath;
	const core::String& baseName = core::string::extractFilename(filename);
	if (defaultAnim[0] != '\0') {
		vxaPath = core::string::path(basePath, core::string::format("%s.%s.vxa", baseName.c_str(), defaultAnim));
		Log::debug("Load the default animation %s", defaultAnim);
	} else if (!entities.empty()) {
		vxaPath = core::string::path(basePath, entities[0].name);
		Log::debug("No default animation set, use the first available vxa: %s", entities[0].name.c_str());
	} else {
		Log::warn("Could not find any vxa file in %s", basePath.c_str());
	}

	for (const io::Filesystem::DirEntry& entry : entities) {
		const core::String &animWithExt = entry.name.substr(baseName.size() + 1);
		const core::String &anim = core::string::extractFilename(animWithExt);
		sceneGraph.addAnimation(anim);
	}

	if (!vxaPath.empty() && !loadVXA(sceneGraph, vxaPath)) {
		Log::warn("Failed to load %s", vxaPath.c_str());
	}

	const uint8_t frameIdx = core::Var::getSafe(cfg::VoxformatFrame)->intVal();
	recursiveTransformVolume(sceneGraph, rootNode, rootNode.transform(frameIdx), frameIdx);

	// some files since version 6 still have stuff here
	return true;
}

void VXRFormat::recursiveTransformVolume(const SceneGraph &sceneGraph, SceneGraphNode &node, const SceneGraphTransform parentTransform, uint8_t frameIdx) {
	SceneGraphTransform currentTransform = node.transform(frameIdx);
	currentTransform.setMatrix(parentTransform.matrix() * node.transform(frameIdx).matrix());
	currentTransform.update();

	if (node.type() == SceneGraphNodeType::Model) {
		node.setTransform(frameIdx, currentTransform, true);
	}

	for (int nodeIdx : node.children()) {
		recursiveTransformVolume(sceneGraph, sceneGraph.node(nodeIdx), currentTransform, frameIdx);
	}
}

bool VXRFormat::loadVXA(SceneGraph& sceneGraph, const core::String& vxaPath) {
	Log::debug("Try to load a vxa file: %s", vxaPath.c_str());
	const io::FilePtr& file = io::filesystem()->open(vxaPath);
	if (!file->validHandle()) {
		return false;
	}
	io::FileStream stream(file);
	VXAFormat format;
	return format.loadGroups(vxaPath, stream, sceneGraph);
}

bool VXRFormat::loadGroups(const core::String &filename, io::SeekableReadStream& stream, SceneGraph& sceneGraph) {
	uint8_t magic[4];
	wrap(stream.readUInt8(magic[0]))
	wrap(stream.readUInt8(magic[1]))
	wrap(stream.readUInt8(magic[2]))
	wrap(stream.readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'R') {
		Log::error("Could not load vxr file: Invalid magic found (%c%c%c%c)",
			magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version;
	if (magic[3] >= '0' && magic[3] <= '9') {
		version = magic[3] - '0';
	} else {
		Log::error("Invalid version found");
		return false;
	}

	Log::debug("Found vxr version: %i", version);

	if (version < 1 || version > 9) {
		Log::error("Could not load vxr file: Unsupported version found (%i)", version);
		return false;
	}

	if (version <= 3) {
		return loadGroupsVersion3AndEarlier(filename, stream, sceneGraph, version);
	}
	return loadGroupsVersion4AndLater(filename, stream, sceneGraph, version);
}

#undef wrap
#undef wrapBool

}

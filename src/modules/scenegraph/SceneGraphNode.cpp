/**
 * @file
 */

#include "SceneGraphNode.h"
#include "core/Assert.h"
#include "core/Color.h"
#include "core/GLM.h"
#include "core/Hash.h"
#include "core/Log.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxelutil/VoxelUtil.h"

namespace scenegraph {

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = InvalidNodeId;
	_uuid = move._uuid;
	move._uuid.clear();
	_referenceId = move._referenceId;
	move._referenceId = InvalidNodeId;
	_palette = core::move(move._palette);
	_parent = move._parent;
	move._parent = InvalidNodeId;
	_pivot = move._pivot;
	_keyFrames = move._keyFrames;
	move._keyFrames = nullptr;
	_keyFramesMap = core::move(move._keyFramesMap);
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	move._type = SceneGraphNodeType::Max;
	_flags = move._flags;
	move._flags &= ~VolumeOwned;
}

SceneGraphNode::~SceneGraphNode() {
	release();
}

SceneGraphNode &SceneGraphNode::operator=(SceneGraphNode &&move) noexcept {
	if (&move == this) {
		return *this;
	}
	setVolume(move._volume, move._flags & VolumeOwned);
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = InvalidNodeId;
	_uuid = move._uuid;
	move._uuid.clear();
	_referenceId = move._referenceId;
	move._referenceId = InvalidNodeId;
	_palette = core::move(move._palette);
	_parent = move._parent;
	move._parent = InvalidNodeId;
	_pivot = move._pivot;
	_keyFrames = move._keyFrames;
	move._keyFrames = nullptr;
	_keyFramesMap = core::move(move._keyFramesMap);
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	_flags = move._flags;
	move._flags &= ~VolumeOwned;
	return *this;
}

SceneGraphNode::SceneGraphNode(SceneGraphNodeType type, const core::String &uuid)
	: _type(type), _flags(VolumeOwned | Visible), _uuid(uuid), _properties(128) {
	if (_uuid.empty()) {
		_uuid = core::generateUUID();
	}
	// ensure that there is at least one animation with keyframes
	setAnimation(DEFAULT_ANIMATION);
}

bool SceneGraphNode::addAnimation(const core::String &anim) {
	if (_keyFramesMap.hasKey(anim)) {
		Log::debug("Animation %s already exists", anim.c_str());
		return false;
	}
	SceneGraphKeyFrames frames;
	frames.emplace_back(SceneGraphKeyFrame{});
	_keyFramesMap.emplace(anim, core::move(frames));
	Log::debug("Added animation %s to node %s (%i)", anim.c_str(), _name.c_str(), _id);
	return true;
}

bool SceneGraphNode::removeAnimation(const core::String &anim) {
	auto iter = _keyFramesMap.find(anim);
	if (iter == _keyFramesMap.end()) {
		return false;
	}
	if (_keyFrames == &iter->value) {
		_keyFrames = nullptr;
	}
	_keyFramesMap.erase(iter);
	if (_keyFramesMap.empty()) {
		setAnimation(DEFAULT_ANIMATION);
	}
	return true;
}

bool SceneGraphNode::setAnimation(const core::String &anim) {
	auto iter = _keyFramesMap.find(anim);
	if (iter == _keyFramesMap.end()) {
		Log::debug("Node %s (%i) doesn't have animation %s yet - adding it now", _name.c_str(), _id, anim.c_str());
		if (!addAnimation(anim)) {
			Log::error("Failed to add animation %s to node '%s' (%i)", anim.c_str(), _name.c_str(), _id);
			return false;
		}
		iter = _keyFramesMap.find(anim);
	}

	if (_keyFrames == &iter->value) {
		return true;
	}

	Log::debug("Switched animation for node %s (%i) to %s", _name.c_str(), _id, anim.c_str());
	_keyFrames = &iter->value;
	core_assert_msg(!_keyFrames->empty(), "Empty keyframes for anim %s", anim.c_str());
	core_assert(keyFramesValidate());
	return true;
}

voxel::Region SceneGraphNode::remapToPalette(const palette::Palette &newPalette, int skipColorIndex) {
	if (type() != SceneGraphNodeType::Model) {
		return voxel::Region::InvalidRegion;
	}
	return voxelutil::remapToPalette(volume(), palette(), newPalette, skipColorIndex);
}

void SceneGraphNode::setPalette(const palette::Palette &palette) {
	if (palette.size() <= 0) {
		return;
	}
	_palette.setValue(palette);
	_palette.value()->markDirty();
}

palette::Palette &SceneGraphNode::palette() const {
	if (!_palette.hasValue()) {
		palette::Palette palette;
		palette.nippon();
		_palette.setValue(palette);
	}
	return *_palette.value();
}

void SceneGraphNode::fixErrors() {
	if (_type == SceneGraphNodeType::Model) {
		if (_volume == nullptr) {
			setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		}
	}
	for (const auto &e : _keyFramesMap) {
		if (e->value.empty()) {
			continue;
		}
		for (SceneGraphKeyFrame &kf : e->value) {
			if (!kf.transform().validate()) {
				kf.transform() = SceneGraphTransform();
			}
		}
	}
}

bool SceneGraphNode::validate() const {
	if (_type == SceneGraphNodeType::Model) {
		if (_volume == nullptr) {
			Log::error("Model node %s (%i) has no volume", _name.c_str(), _id);
			return false;
		}
	}
	if (_type == SceneGraphNodeType::ModelReference) {
		if (_referenceId == InvalidNodeId) {
			Log::error("Model reference node %s (%i) has no reference", _name.c_str(), _id);
			return false;
		}
	}
	for (const auto &e : _keyFramesMap) {
		if (e->value.empty()) {
			continue;
		}
		for (const SceneGraphKeyFrame &kf : e->value) {
			if (!kf.transform().validate()) {
				Log::error("Invalid keyframe %i for node %s (%i)", kf.frameIdx, _name.c_str(), _id);
				return false;
			}
		}
	}
	return true;
}

bool SceneGraphNode::setPivot(const glm::vec3 &pivot) {
	glm_assert_vec3(pivot);
	_pivot = pivot;
	return true;
}

const glm::vec3 &SceneGraphNode::pivot() const {
	return _pivot;
}

glm::vec3 SceneGraphNode::worldPivot() const {
	const voxel::Region &r = region();
	return r.getLowerCornerf() + _pivot * glm::vec3(r.getDimensionsInVoxels());
}

void SceneGraphNode::translate(const glm::vec3 &translation) {
	Log::debug("Translate the node by %f %f %f", translation.x, translation.y, translation.z);
	for (auto *keyFrames : _keyFramesMap) {
		for (SceneGraphKeyFrame &keyFrame : keyFrames->value) {
			SceneGraphTransform &transform = keyFrame.transform();
			const glm::vec3 &t =
				transform.localTranslation() + glm::conjugate(transform.localOrientation()) * translation;
			transform.setLocalTranslation(t);
		}
	}
}

void SceneGraphNode::setTranslation(const glm::vec3 &translation, bool world) {
	for (auto *keyFrames : _keyFramesMap) {
		for (SceneGraphKeyFrame &keyFrame : keyFrames->value) {
			SceneGraphTransform &transform = keyFrame.transform();
			if (world) {
				transform.setWorldTranslation(translation);
			} else {
				transform.setLocalTranslation(translation);
			}
		}
	}
}

void SceneGraphNode::setRotation(const glm::quat &rotation, bool world) {
	for (auto *keyFrames : _keyFramesMap) {
		for (SceneGraphKeyFrame &keyFrame : keyFrames->value) {
			SceneGraphTransform &transform = keyFrame.transform();
			if (world) {
				transform.setWorldOrientation(rotation);
			} else {
				transform.setLocalOrientation(rotation);
			}
		}
	}
}

void SceneGraphNode::release() {
	if (_flags & VolumeOwned) {
		delete _volume;
		releaseOwnership();
	}
	_volume = nullptr;
}

void SceneGraphNode::releaseOwnership() {
	_flags &= ~VolumeOwned;
}

void SceneGraphNode::setVolume(voxel::RawVolume *volume, bool transferOwnership) {
	core_assert_msg(_type == SceneGraphNodeType::Model, "Expected to get a model node, but got a node with type %i",
					(int)_type);
	release();
	if (transferOwnership) {
		_flags |= VolumeOwned;
	} else {
		_flags &= ~VolumeOwned;
	}
	_volume = volume;
}

void SceneGraphNode::setVolume(const voxel::RawVolume *volume) {
	core_assert_msg(_type == SceneGraphNodeType::Model, "Expected to get a model node, but got a node with type %i",
					(int)_type);
	release();
	_volume = (voxel::RawVolume *)volume;
}

bool SceneGraphNode::isReference() const {
	return _type == SceneGraphNodeType::ModelReference;
}

bool SceneGraphNode::isAnyModelNode() const {
	return _type == SceneGraphNodeType::Model || _type == SceneGraphNodeType::ModelReference;
}

bool SceneGraphNode::isModelNode() const {
	return _type == SceneGraphNodeType::Model;
}

bool SceneGraphNode::isReferenceable() const {
	return _type == SceneGraphNodeType::Model;
}

int SceneGraphNode::reference() const {
	return _referenceId;
}

bool SceneGraphNode::setReference(int nodeId, bool forceChangeNodeType) {
	if (_type != SceneGraphNodeType::ModelReference) {
		if (forceChangeNodeType) {
			setVolume(nullptr, false);
			_type = SceneGraphNodeType::ModelReference;
		} else {
			return false;
		}
	}
	_referenceId = nodeId;
	return true;
}

bool SceneGraphNode::unreferenceModelNode(const SceneGraphNode &node) {
	if (_type != SceneGraphNodeType::ModelReference) {
		Log::error("Failed to unreference - %i is no reference node", _id);
		return false;
	}
	core_assert(_referenceId != InvalidNodeId);
	if (node.type() != SceneGraphNodeType::Model) {
		Log::error("Failed to unreference - node %i is no model node", node.id());
		return false;
	}
	if (node.id() != _referenceId) {
		Log::error("This node wasn't referenced - can't unreference from %i, expected %i", node.id(), reference());
		return false;
	}
	_type = SceneGraphNodeType::Model;
	_referenceId = InvalidNodeId;
	setVolume(new voxel::RawVolume(node.volume()), true);
	setPalette(node.palette());
	return true;
}

const voxel::Region &SceneGraphNode::region() const {
	if (_volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	return _volume->region();
}

voxel::Region SceneGraphNode::sceneRegion(const voxel::Region &volumeRegion, const glm::vec3 &pivot,
										  KeyFrameIndex keyFrameIdx) const {
	const SceneGraphTransform &transform = this->transform(keyFrameIdx);
	const glm::vec3 &scale = transform.worldScale();
	// TODO: rotation
	const glm::vec3 translation = transform.worldTranslation() - pivot * glm::vec3(volumeRegion.getDimensionsInVoxels());
	const glm::vec3 mins = (volumeRegion.getLowerCornerf() + translation) * scale;
	const glm::vec3 maxs = mins + glm::vec3(volumeRegion.getDimensionsInCells());
	return {glm::floor(mins), glm::ceil(maxs)};
}

bool SceneGraphNode::isLeaf() const {
	return _children.empty();
}

bool SceneGraphNode::addChild(int id) {
	for (const int childId : _children) {
		if (childId == id) {
			return false;
		}
	}
	_children.push_back(id);
	return true;
}

bool SceneGraphNode::removeChild(int id) {
	const int n = (int)_children.size();
	for (int i = 0; i < n; ++i) {
		if (_children[i] == id) {
			_children.erase(i);
			return true;
		}
	}
	return false;
}

const SceneGraphNodeChildren &SceneGraphNode::children() const {
	return _children;
}

const core::StringMap<core::String> &SceneGraphNode::properties() const {
	return _properties;
}

core::StringMap<core::String> &SceneGraphNode::properties() {
	return _properties;
}

core::String SceneGraphNode::property(const core::String &key) const {
	core::String value;
	_properties.get(key, value);
	return value;
}

float SceneGraphNode::propertyf(const core::String &key) const {
	return property(key).toFloat();
}

void SceneGraphNode::addProperties(const core::StringMap<core::String> &map) {
	for (const auto &entry : map) {
		setProperty(entry->key, entry->value);
	}
}

bool SceneGraphNode::setProperty(const core::String &key, const char *value) {
	if (_properties.size() >= _properties.capacity()) {
		return false;
	}
	_properties.put(key, value);
	return true;
}

bool SceneGraphNode::setProperty(const core::String &key, bool value) {
	if (_properties.size() >= _properties.capacity()) {
		return false;
	}
	_properties.put(key, core::string::toString(value));
	return true;
}

bool SceneGraphNode::setProperty(const core::String& key, float value) {
	return setProperty(key, core::string::toString(value));
}

bool SceneGraphNode::setProperty(const core::String& key, uint32_t value) {
	return setProperty(key, core::string::toString(value));
}

bool SceneGraphNode::setProperty(const core::String& key, core::RGBA value) {
	return setProperty(key, core::Color::toHex(value, true));
}

bool SceneGraphNode::setProperty(const core::String &key, const core::String &value) {
	if (_properties.size() >= _properties.capacity()) {
		return false;
	}
	auto iter = _properties.find(key);
	if (iter != _properties.end()) {
		if (iter->value == value) {
			return false;
		}
	}
	_properties.put(key, value);
	return true;
}

SceneGraphKeyFrame &SceneGraphNode::keyFrame(KeyFrameIndex keyFrameIdx) {
	SceneGraphKeyFrames *kfs = keyFrames();
	core_assert(kfs != nullptr);
	if ((int)kfs->size() <= keyFrameIdx) {
		kfs->resize(keyFrameIdx + 1);
	}
	return (*kfs)[keyFrameIdx];
}

const SceneGraphKeyFrame *SceneGraphNode::keyFrame(KeyFrameIndex keyFrameIdx) const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	if ((int)kfs.size() <= keyFrameIdx) {
		return nullptr;
	}
	return &kfs[keyFrameIdx];
}

bool SceneGraphNode::keyFramesValidate() const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	if (kfs.empty()) {
		Log::error("Invalid key frames: We need at least one key frame for each animation");
		return false;
	}
	int lastKeyFrameIdx = -1;
	for (const SceneGraphKeyFrame &kf : kfs) {
		if (kf.frameIdx < 0) {
			Log::error("Invalid key frames: index is invalid: %i", kf.frameIdx);
			return false;
		}
		if (kf.frameIdx <= lastKeyFrameIdx) {
			Log::error("Invalid key frames: index is not sorted: %i <= %i", kf.frameIdx, lastKeyFrameIdx);
			return false;
		}
		lastKeyFrameIdx = kf.frameIdx;
	}
	return true;
}

SceneGraphTransform &SceneGraphNode::transform(KeyFrameIndex keyFrameIdx) {
	return keyFrame(keyFrameIdx).transform();
}

const SceneGraphTransform &SceneGraphNode::transform(KeyFrameIndex keyFrameIdx) const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	while (keyFrameIdx > 0 && keyFrameIdx >= (int)kfs.size()) {
		--keyFrameIdx;
	}
	return kfs[keyFrameIdx].transform();
}

void SceneGraphNode::setTransform(KeyFrameIndex keyFrameIdx, const SceneGraphTransform &transform) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(keyFrameIdx);
	nodeFrame.setTransform(transform);
}

const SceneGraphKeyFrames &SceneGraphNode::keyFrames() const {
	static SceneGraphKeyFrames kfDummy{SceneGraphKeyFrame{}};
	if (_keyFrames == nullptr) {
		Log::error("No animation set for node '%s' (%i)", _name.c_str(), _id);
		return kfDummy;
	}
	return *_keyFrames;
}

const SceneGraphKeyFrames &SceneGraphNode::keyFrames(const core::String &anim) const {
	static SceneGraphKeyFrames kfDummy{SceneGraphKeyFrame{}};
	auto iter = _keyFramesMap.find(anim);
	if (iter == _keyFramesMap.end()) {
		Log::error("No keyframes for animation '%s'", anim.c_str());
		return kfDummy;
	}
	return iter->value;
}

SceneGraphKeyFrames *SceneGraphNode::keyFrames() {
	return _keyFrames;
}

bool SceneGraphNode::hasActiveAnimation() const {
	return _keyFrames != nullptr;
}

bool SceneGraphNode::hasKeyFrame(FrameIndex frameIdx) const {
	const SceneGraphKeyFrames *kfs = _keyFrames;
	if (kfs == nullptr) {
		return false;
	}
	for (size_t i = 0; i < kfs->size(); ++i) {
		const SceneGraphKeyFrame &kf = (*kfs)[i];
		if (kf.frameIdx == frameIdx) {
			return true;
		}
	}
	return false;
}

KeyFrameIndex SceneGraphNode::addKeyFrame(FrameIndex frameIdx) {
	SceneGraphKeyFrames *kfs = keyFrames();
	if (kfs == nullptr) {
		return InvalidKeyFrame;
	}
	for (size_t i = 0; i < kfs->size(); ++i) {
		const SceneGraphKeyFrame &kf = (*kfs)[i];
		if (kf.frameIdx == frameIdx) {
			Log::debug("keyframe already exists at index %i", (int)i);
			return InvalidKeyFrame;
		}
	}

	SceneGraphKeyFrame keyFrame;
	keyFrame.frameIdx = frameIdx;
	kfs->push_back(keyFrame);
	sortKeyFrames();
	size_t i = 0;
	for (; i < kfs->size(); ++i) {
		const SceneGraphKeyFrame &kf = (*kfs)[i];
		if (kf.frameIdx == frameIdx) {
			break;
		}
	}
	core_assert(i != kfs->size());
	return i;
}

void SceneGraphNode::sortKeyFrames() {
	static auto frameSorter = [](const SceneGraphKeyFrame &a, const SceneGraphKeyFrame &b) {
		return a.frameIdx > b.frameIdx;
	};
	if (SceneGraphKeyFrames *kfs = keyFrames()) {
		kfs->sort(frameSorter);
	}
}

bool SceneGraphNode::removeKeyFrame(FrameIndex frameIdx) {
	const SceneGraphKeyFrames *kfs = keyFrames();
	if (kfs == nullptr || kfs->size() <= 1) {
		return false;
	}
	const KeyFrameIndex keyFrameIdx = keyFrameForFrame(frameIdx);
	return removeKeyFrameByIndex(keyFrameIdx);
}

bool SceneGraphNode::removeKeyFrameByIndex(KeyFrameIndex keyFrameIdx) {
	SceneGraphKeyFrames *kfs = keyFrames();
	if (kfs == nullptr || kfs->size() <= 1) {
		return false;
	}
	kfs->erase(keyFrameIdx);
	return true;
}

bool SceneGraphNode::duplicateKeyFrames(const core::String &fromAnimation, const core::String &toAnimation) {
	_keyFramesMap.put(toAnimation, keyFrames(fromAnimation));
	return true;
}

bool SceneGraphNode::setKeyFrames(const SceneGraphKeyFrames &kf) {
	if (kf.empty()) {
		return false;
	}
	if (SceneGraphKeyFrames *kfs = keyFrames()) {
		*kfs = kf;
		return true;
	}
	return false;
}

void SceneGraphNode::setAllKeyFrames(const SceneGraphKeyFramesMap &map, const core::String &animation) {
	_keyFramesMap = map;
	setAnimation(animation);
}

const SceneGraphKeyFramesMap &SceneGraphNode::allKeyFrames() const {
	return _keyFramesMap;
}

SceneGraphKeyFramesMap &SceneGraphNode::allKeyFrames() {
	return _keyFramesMap;
}

bool SceneGraphNode::hasKeyFrameForFrame(FrameIndex frameIdx, KeyFrameIndex *existingIndex) const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	const KeyFrameIndex n = (KeyFrameIndex)kfs.size();
	for (KeyFrameIndex i = 0; i < n; ++i) {
		const SceneGraphKeyFrame &kf = kfs[i];
		if (kf.frameIdx == frameIdx) {
			if (existingIndex) {
				*existingIndex = i;
			}
			return true;
		}
	}
	return false;
}

KeyFrameIndex SceneGraphNode::nextKeyFrameForFrame(FrameIndex frameIdx) const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	// this assumes that the key frames are sorted by their frame
	const int n = (int)kfs.size();
	core_assert(n > 0);
	int closest = 0;
	for (int i = 0; i < n; ++i) {
		const SceneGraphKeyFrame &kf = kfs[i];
		if (kf.frameIdx <= frameIdx) {
			closest = i;
		} else {
			return i;
		}
	}
	return closest;
}

KeyFrameIndex SceneGraphNode::previousKeyFrameForFrame(FrameIndex frameIdx) const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	// this assumes that the key frames are sorted by their frame
	const int n = (int)kfs.size();
	core_assert(n > 0);
	int closest = 0;
	for (int i = 0; i < n; ++i) {
		const SceneGraphKeyFrame &kf = kfs[i];
		if (kf.frameIdx < frameIdx) {
			closest = i;
		}
	}
	return closest;
}

KeyFrameIndex SceneGraphNode::keyFrameForFrame(FrameIndex frameIdx) const {
	const SceneGraphKeyFrames &kfs = keyFrames();
	// this assumes that the key frames are sorted by their frame
	const int n = (int)kfs.size();
	core_assert(n > 0);
	for (int i = 0; i < n; ++i) {
		const SceneGraphKeyFrame &kf = kfs[i];
		if (kf.frameIdx == frameIdx) {
			return i;
		} else if (kf.frameIdx > frameIdx) {
			if (i == 0) {
				return 0;
			}
			return i - 1;
		}
	}
	return n - 1;
}

FrameIndex SceneGraphNode::maxFrame(const core::String &animation) const {
	FrameIndex maxFrameIdx = 0;
	const SceneGraphKeyFrames &kfs = keyFrames(animation);
	for (const auto &keyframe : kfs) {
		maxFrameIdx = core_max(keyframe.frameIdx, maxFrameIdx);
	}
	return maxFrameIdx;
}

SceneGraphNodeCamera::SceneGraphNodeCamera(const core::String &uuid) : Super(SceneGraphNodeType::Camera, uuid) {
}

float SceneGraphNodeCamera::farPlane() const {
	return propertyf(PropFarPlane);
}

void SceneGraphNodeCamera::setFarPlane(float val) {
	setProperty(PropFarPlane, core::string::toString(val));
}

float SceneGraphNodeCamera::nearPlane() const {
	return propertyf(PropNearPlane);
}

void SceneGraphNodeCamera::setNearPlane(float val) {
	setProperty(PropNearPlane, core::string::toString(val));
}

bool SceneGraphNodeCamera::isOrthographic() const {
	return property(PropMode) == Modes[0];
}

void SceneGraphNodeCamera::setOrthographic() {
	setProperty(PropMode, Modes[0]);
}

bool SceneGraphNodeCamera::isPerspective() const {
	return property(PropMode) == Modes[1];
}

void SceneGraphNodeCamera::setPerspective() {
	setProperty(PropMode, Modes[1]);
}

int SceneGraphNodeCamera::width() const {
	return property(PropWidth).toInt();
}

void SceneGraphNodeCamera::setWidth(int val) {
	setProperty(PropWidth, core::string::toString(val));
}

int SceneGraphNodeCamera::height() const {
	return property(PropHeight).toInt();
}

void SceneGraphNodeCamera::setHeight(int val) {
	setProperty(PropHeight, core::string::toString(val));
}

int SceneGraphNodeCamera::fieldOfView() const {
	return property(PropFov).toInt();
}

void SceneGraphNodeCamera::setFieldOfView(int val) {
	setProperty(PropFov, core::string::toString(val));
}

float SceneGraphNodeCamera::aspectRatio() const {
	return property(PropAspect).toFloat();
}

void SceneGraphNodeCamera::setAspectRatio(float val) {
	setProperty(PropAspect, core::string::toString(val));
}

} // namespace scenegraph

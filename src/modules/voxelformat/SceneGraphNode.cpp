/**
 * @file
 */

#include "SceneGraphNode.h"
#include "core/Assert.h"
#include "core/Log.h"
#include "voxel/RawVolume.h"

#include <glm/ext/scalar_constants.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/transform.hpp>

namespace voxelformat {

SceneGraphNode::SceneGraphNode(SceneGraphNodeType type) : _type(type) {
	// ensure that there is at least one frame
	SceneGraphKeyFrame frame;
	_keyFrames.emplace_back(frame);
}

void SceneGraphTransform::setPivot(const glm::vec3 &normalizedPivot) {
	_dirty |= DIRTY_PIVOT;
	_normalizedPivot = normalizedPivot;
#if 0
	core_assert(glm::all(glm::lessThanEqual(normalizedPivot, glm::vec3(1.0f))));
	core_assert(glm::all(glm::greaterThanEqual(normalizedPivot, glm::vec3(0.0f))));
#endif
}

void SceneGraphTransform::setTranslation(const glm::vec3 &translation) {
	_dirty |= DIRTY_TRANSLATION;
	_translation = translation;
}

void SceneGraphTransform::setOrientation(const glm::quat &orientation) {
	_dirty |= DIRTY_ORIENTATION;
	_orientation = orientation;
}

void SceneGraphTransform::setScale(float scale) {
	_dirty |= DIRTY_SCALE;
	_scale = scale;
}

void SceneGraphTransform::setMatrix(const glm::mat4x4 &matrix) {
	_dirty |= DIRTY_MATRIX;
	_mat = matrix;
}

const glm::mat4x4 &SceneGraphTransform::matrix() const {
	return _mat;
}

const glm::vec3 &SceneGraphTransform::pivot() const {
	return _normalizedPivot;
}

const glm::vec3 &SceneGraphTransform::translation() const {
	return _translation;
}

const glm::quat &SceneGraphTransform::orientation() const {
	return _orientation;
}

float SceneGraphTransform::scale() const {
	return _scale;
}

void SceneGraphTransform::update() {
	if (_dirty == 0u) {
		return;
	}
	if (_dirty & DIRTY_MATRIX) {
		core_assert((_dirty & (DIRTY_TRANSLATION | DIRTY_ORIENTATION | DIRTY_SCALE)) == 0u);
		_translation = _mat[3];
		_scale = glm::length(glm::vec3(_mat[0]));
		if (glm::abs(_scale) < glm::epsilon<float>()) {
			_scale = 1.0f;
		}
		const glm::mat3 rotMtx(glm::vec3(_mat[0]) / _scale, glm::vec3(_mat[1]) / _scale, glm::vec3(_mat[2]) / _scale);
		_orientation = glm::quat_cast(rotMtx);
	} else if (_dirty & (DIRTY_TRANSLATION | DIRTY_ORIENTATION | DIRTY_SCALE)) {
		core_assert((_dirty & DIRTY_MATRIX) == 0u);
		_mat = glm::translate(_translation) * glm::mat4_cast(_orientation) * glm::scale(glm::vec3(_scale));
	}
	_dirty = 0u;
}

glm::vec3 SceneGraphTransform::apply(const glm::vec3 &pos, const glm::vec3 &size) const {
	core_assert_msg((_dirty & (DIRTY_TRANSLATION | DIRTY_ORIENTATION | DIRTY_SCALE)) == 0u, "Missing update for transform matrix %i", (int)_dirty);
	return glm::vec3(_mat * (glm::vec4(pos, 1.0f) - glm::vec4(_normalizedPivot * size, 0.0f)));
}

SceneGraphNode::SceneGraphNode(SceneGraphNode &&move) noexcept {
	_volume = move._volume;
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = -1;
	_parent = move._parent;
	move._parent = -1;
	_keyFrames = core::move(move._keyFrames);
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	move._type = SceneGraphNodeType::Max;
	_visible = move._visible;
	_locked = move._locked;
	_volumeOwned = move._volumeOwned;
	move._volumeOwned = false;
}

SceneGraphNode &SceneGraphNode::operator=(SceneGraphNode &&move) noexcept {
	if (&move == this) {
		return *this;
	}
	setVolume(move._volume, move._volumeOwned);
	move._volume = nullptr;
	_name = core::move(move._name);
	_id = move._id;
	move._id = -1;
	_parent = move._parent;
	move._parent = -1;
	_keyFrames = core::move(move._keyFrames);
	_properties = core::move(move._properties);
	_children = core::move(move._children);
	_type = move._type;
	_visible = move._visible;
	_locked = move._locked;
	move._volumeOwned = false;
	return *this;
}

void SceneGraphNode::release() {
	if (_volumeOwned) {
		delete _volume;
	}
	_volume = nullptr;
}

void SceneGraphNode::releaseOwnership() {
	_volumeOwned = false;
}

void SceneGraphNode::setVolume(voxel::RawVolume *volume, bool transferOwnership) {
	release();
	_volumeOwned = transferOwnership;
	_volume = volume;
}

void SceneGraphNode::setVolume(const voxel::RawVolume *volume, bool transferOwnership) {
	release();
	_volumeOwned = transferOwnership;
	_volume = (voxel::RawVolume *)volume;
}

const voxel::Region &SceneGraphNode::region() const {
	if (_volume == nullptr) {
		return voxel::Region::InvalidRegion;
	}
	return _volume->region();
}

void SceneGraphNode::translate(const glm::ivec3 &v) {
	if (_volume != nullptr) {
		_volume->translate(v);
	}
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

const core::Buffer<int, 32> &SceneGraphNode::children() const {
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

void SceneGraphNode::addProperties(const core::StringMap<core::String> &map) {
	for (const auto &entry : map) {
		setProperty(entry->key, entry->value);
	}
}

void SceneGraphNode::setTransform(uint8_t frameIdx, const SceneGraphTransform &transform, bool updateMatrix) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(frameIdx);
	nodeFrame.transform = transform;
	if (updateMatrix) {
		nodeFrame.transform.update();
	}
}

void SceneGraphNode::setPivot(uint8_t frameIdx, const glm::ivec3 &pos, const glm::ivec3 &size) {
	SceneGraphKeyFrame &nodeFrame = keyFrame(frameIdx);
	nodeFrame.transform.setPivot(glm::vec3(pos) / glm::vec3(size));
}

const core::DynamicArray<SceneGraphKeyFrame> &SceneGraphNode::keyFrames() const {
	return _keyFrames;
}

void SceneGraphNode::setKeyFrames(const core::DynamicArray<SceneGraphKeyFrame> &kf) {
	_keyFrames = kf;
}

} // namespace voxelformat

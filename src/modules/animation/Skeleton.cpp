/**
 * @file
 */

#include "Skeleton.h"
#include "animation/BoneUtil.h"

namespace animation {

Skeleton::Skeleton() {
	_bones.fill(Bone());
}

const Bone& Skeleton::bone(BoneId id) const {
	return _bones[core::enumVal(id)];
}

Bone& Skeleton::bone(BoneId id) {
	return _bones[core::enumVal(id)];
}

Bone& Skeleton::torsoBone(float scale) {
	Bone& torso = bone(BoneId::Torso);
	torso.scale = glm::vec3(_private::torsoScale * scale);
	torso.translation = glm::zero<glm::vec3>();
	torso.orientation = glm::quat_identity<float, glm::defaultp>();
	return torso;
}

void Skeleton::lerp(const Skeleton& previous, double deltaFrameSeconds) {
	for (int i = 0; i < core::enumVal(BoneId::Max); ++i) {
		const BoneId id = (BoneId)i;
		bone(id).lerp(previous.bone(id), deltaFrameSeconds);
	}
}

}

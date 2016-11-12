#pragma once

#include "core/Common.h"
#include "voxel/polyvox/Region.h"
#include "voxel/polyvox/PagedVolume.h"

namespace voxel {

class GeneratorContext {
private:
	PagedVolume* _pagedVolume;
	PagedVolume::Chunk* _chunk;
	Region _validRegion;
public:
	Region region;
	Region maxRegion = Region::MaxRegion;

	GeneratorContext(PagedVolume* voxelStorage, PagedVolume::Chunk* chunk, const Region& _region) :
			_pagedVolume(voxelStorage), _chunk(chunk), region(_region) {
		if (_chunk != nullptr) {
			_validRegion = _chunk->getRegion();
		}
	}

	inline PagedVolume::Chunk* getChunk() const {
		return _chunk;
	}

	inline PagedVolume* getVolume() const {
		return _pagedVolume;
	}

	inline bool setVoxel(const glm::ivec3& pos, const Voxel& voxel) {
		return setVoxel(pos.x, pos.y, pos.z, voxel);
	}

	inline const Voxel& getVoxel(const glm::ivec3& pos) const {
		return getVoxel(pos.x, pos.y, pos.z);
	}

	inline const Voxel& getVoxel(int x, int y, int z) const {
		if (_validRegion.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			return _chunk->getVoxel(x - _validRegion.getLowerX(), y - _validRegion.getLowerY(), z - _validRegion.getLowerZ());
		}
		core_assert_msg(maxRegion.containsPoint(x, y, z), "the accessed voxel exceeds the max bounds of %i:%i:%i/%i:%i:%i (voxel was at %i:%i:%i)",
				maxRegion.getLowerX(), maxRegion.getLowerY(), maxRegion.getLowerZ(), maxRegion.getUpperX(), maxRegion.getUpperY(), maxRegion.getUpperZ(), x, y, z);
		core_assert(_pagedVolume != nullptr);
		return _pagedVolume->getVoxel(x, y, z);
	}

	inline bool setVoxel(int x, int y, int z, const Voxel& voxel) {
		if (_validRegion.containsPoint(x, y, z)) {
			core_assert(_chunk != nullptr);
			_chunk->setVoxel(x - _validRegion.getLowerX(), y - _validRegion.getLowerY(), z - _validRegion.getLowerZ(), voxel);
			return true;
		} else if (maxRegion.containsPoint(x, y, z)) {
			core_assert(_pagedVolume != nullptr);
			_pagedVolume->setVoxel(x, y, z, voxel);
			return true;
		}
		return false;
	}

	inline bool setVoxels(int x, int z, const Voxel* voxels, int amount) {
		if (_validRegion.containsPoint(x, 0, z)) {
			// first part goes into the chunk
			const int w = _validRegion.getWidthInVoxels();
			_chunk->setVoxels(x - _validRegion.getLowerX(), z - _validRegion.getLowerZ(), voxels, std::min(w, amount));
			amount -= w;
			if (amount > 0) {
				// everything else goes into the volume
				core_assert(_pagedVolume != nullptr);
				_pagedVolume->setVoxels(x, z, voxels + w, amount);
			}
			return true;
		} else if (maxRegion.containsPoint(x, 0, z)) {
			// TODO: add region/chunk support here, too
			core_assert(_pagedVolume != nullptr);
			_pagedVolume->setVoxels(x, z, voxels, amount);
			return true;
		}
		return false;
	}
};

}

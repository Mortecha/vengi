/**
 * @file
 */

#pragma once

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "core/Color.h"
#include "voxel/PagedVolumeWrapper.h"
#include "core/Log.h"
#include "voxel/PagedVolume.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/MaterialColor.h"
#include "voxel/Palette.h"
#include "voxel/Constants.h"
#include "math/Random.h"
#include "core/Common.h"
#include "voxelutil/VolumeVisitor.h"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

namespace voxel {

static int VolumePrintThreshold = 10;

template<typename Volume>
inline int countVoxels(const Volume& volume, const voxel::Voxel &voxel) {
	int cnt = 0;
	voxelutil::visitVolume(volume, [&](int, int, int, const voxel::Voxel &v) {
		if (v == voxel) {
			++cnt;
		}
	}, voxelutil::VisitAll());
	return cnt;
}

inline void volumeComparator(const voxel::RawVolume& volume1, const voxel::Palette &pal1, const voxel::RawVolume& volume2, const voxel::Palette &pal2, bool includingColor, bool includingRegion) {
	const Region& r1 = volume1.region();
	const Region& r2 = volume2.region();
	if (includingRegion) {
		ASSERT_EQ(r1, r2) << "regions differ: " << r1.toString() << " vs " << r2.toString();
	}

	const int32_t lowerX = r1.getLowerX();
	const int32_t lowerY = r1.getLowerY();
	const int32_t lowerZ = r1.getLowerZ();
	const int32_t upperX = r1.getUpperX();
	const int32_t upperY = r1.getUpperY();
	const int32_t upperZ = r1.getUpperZ();
	const int32_t lower2X = r2.getLowerX();
	const int32_t lower2Y = r2.getLowerY();
	const int32_t lower2Z = r2.getLowerZ();
	const int32_t upper2X = r2.getUpperX();
	const int32_t upper2Y = r2.getUpperY();
	const int32_t upper2Z = r2.getUpperZ();

	voxel::RawVolume::Sampler s1(volume1);
	voxel::RawVolume::Sampler s2(volume2);
	for (int32_t z1 = lowerZ, z2 = lower2Z; z1 <= upperZ && z2 <= upper2Z; ++z1, ++z2) {
		for (int32_t y1 = lowerY, y2 = lower2Y; y1 <= upperY && y2 <= upper2Y; ++y1, ++y2) {
			for (int32_t x1 = lowerX, x2 = lower2X; x1 <= upperX && x2 <= upper2X; ++x1, ++x2) {
				s1.setPosition(x1, y1, z1);
				s2.setPosition(x2, y2, z2);
				const voxel::Voxel& voxel1 = s1.voxel();
				const voxel::Voxel& voxel2 = s2.voxel();
				ASSERT_EQ(voxel1.getMaterial(), voxel2.getMaterial())
					<< "Voxel differs at " << x1 << ":" << y1 << ":" << z1 << " in material - voxel1["
					<< voxel::VoxelTypeStr[(int)voxel1.getMaterial()] << ", " << (int)voxel1.getColor() << "], voxel2["
					<< voxel::VoxelTypeStr[(int)voxel2.getMaterial()] << ", " << (int)voxel2.getColor() << "]";
				if (voxel::isAir(voxel1.getMaterial())) {
					continue;
				}
				if (!includingColor) {
					continue;
				}
				const core::RGBA& c1 = pal1.colors[voxel1.getColor()];
				const core::RGBA& c2 = pal2.colors[voxel2.getColor()];
				const float delta = core::Color::getDistance(c1, c2);
				ASSERT_LT(delta, 0.001f) << "Voxel differs at " << x1 << ":" << y1 << ":" << z1
										 << " in material - voxel1[" << voxel::VoxelTypeStr[(int)voxel1.getMaterial()]
										 << ", " << (int)voxel1.getColor() << "], voxel2["
										 << voxel::VoxelTypeStr[(int)voxel2.getMaterial()] << ", "
										 << (int)voxel2.getColor() << "], color1[" << core::Color::toHex(c1)
										 << "], color2[" << core::Color::toHex(c2) << "], delta[" << delta << "]";
			}
		}
	}
}

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::Region& region) {
	return os << "region["
			<< "mins(" << glm::to_string(region.getLowerCorner()) << "), "
			<< "maxs(" << glm::to_string(region.getUpperCorner()) << ")"
			<< "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::Voxel& voxel) {
	return os << "voxel[" << voxel::VoxelTypeStr[(int)voxel.getMaterial()] << ", " << (int)voxel.getColor() << "]";
}

inline ::std::ostream& operator<<(::std::ostream& os, const voxel::RawVolume& volume) {
	const voxel::Region& region = volume.region();
	os << "volume[" << region;
	if (volume.depth() <= VolumePrintThreshold && volume.width() <= VolumePrintThreshold && volume.height() <= VolumePrintThreshold) {
		const int32_t lowerX = region.getLowerX();
		const int32_t lowerY = region.getLowerY();
		const int32_t lowerZ = region.getLowerZ();
		const int32_t upperX = region.getUpperX();
		const int32_t upperY = region.getUpperY();
		const int32_t upperZ = region.getUpperZ();
		os << "\n";
		for (int32_t z = lowerZ; z <= upperZ; ++z) {
			for (int32_t y = lowerY; y <= upperY; ++y) {
				for (int32_t x = lowerX; x <= upperX; ++x) {
					const glm::ivec3 pos(x, y, z);
					const voxel::Voxel& voxel = volume.voxel(pos);
					os << x << ", " << y << ", " << z << ": " << voxel << "\n";
				}
			}
		}
	}
	os << "]";
	return os;
}

}

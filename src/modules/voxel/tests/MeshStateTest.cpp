/**
 * @file
 */

#include "voxel/MeshState.h"
#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "palette/Palette.h"
#include "voxel/MaterialColor.h"
#include "voxel/SurfaceExtractor.h"

namespace voxel {

class MeshStateTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	void SetUp() override {
		Super::SetUp();
		core::Var::get(cfg::VoxelMeshSize, "16", core::CV_READONLY);
		core::Var::get(cfg::VoxelMeshMode, core::string::toString((int)voxel::SurfaceExtractionType::Cubic));
	}
};

TEST_F(MeshStateTest, testExtractRegion) {
	voxel::RawVolume v(voxel::Region(-1, 1));

	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();
	(void)meshState.setVolume(0, &v, &pal, true, deleted);

	EXPECT_EQ(0, meshState.pendingExtractions());
	const voxel::Region region(1, 0, 1, 1, 0, 1);
	meshState.scheduleRegionExtraction(0, region);
	EXPECT_EQ(1, meshState.pendingExtractions());

	(void)meshState.shutdown();
}

TEST_F(MeshStateTest, testExtractRegionBoundary) {
	voxel::RawVolume v(voxel::Region(0, 31));

	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();
	(void)meshState.setVolume(0, &v, &pal, true, deleted);

	EXPECT_EQ(0, meshState.pendingExtractions());
	// worst case scenario - touching all adjacent regions
	const voxel::Region region(15, 15);
	meshState.scheduleRegionExtraction(0, region);
	EXPECT_EQ(8, meshState.pendingExtractions());

	const voxel::Region region2(14, 14);
	meshState.scheduleRegionExtraction(0, region2);
	EXPECT_EQ(9, meshState.pendingExtractions());
	(void)meshState.shutdown();
}

// https://github.com/vengi-voxel/vengi/issues/445
TEST_F(MeshStateTest, testExtractRegionBoundaryMeshesIssue445) {
	glm::ivec3 mins(-1, 0, -1);
	glm::ivec3 maxs(3, 1, 3);
	voxel::Region region(mins, maxs);
	voxel::RawVolume v(region);
	MeshState meshState;
	meshState.construct();
	meshState.init();
	bool deleted = false;
	palette::Palette pal;
	pal.nippon();

	for (int x = mins.x; x <= maxs.x; ++x) {
		for (int y = mins.y; y <= maxs.y; ++y) {
			for (int z = mins.z; z <= maxs.z; ++z) {
				v.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, 1));
			}
		}
	}

	(void)meshState.setVolume(0, &v, &pal, true, deleted);
	meshState.scheduleRegionExtraction(0, region);
	EXPECT_EQ(8, meshState.pendingExtractions());
	meshState.extractAllPending();
	(void)meshState.shutdown();
}

} // namespace voxelrender

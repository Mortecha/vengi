/**
 * @file
 */

#include "TreeGenerator.h"
#include "LSystemGenerator.h"
#include "voxel/WorldContext.h"
#include "voxel/Voxel.h"

namespace voxel {

int TreeGenerator::findFloor(const TerrainContext& ctx, int x, int z) {
	for (int i = MAX_TERRAIN_HEIGHT - 1; i >= MAX_WATER_HEIGHT; i--) {
		const int material = ctx.getVoxel(x, i, z).getMaterial();
		if (isLeaves(material)) {
			return -1;
		}
		if (!isRock(material) && (isFloor(material) || isWood(material))) {
			return i + 1;
		}
	}
	return -1;
}

void TreeGenerator::createTrees(TerrainContext& ctx, const BiomeManager& biomManager, core::Random& random) {
	const Region& region = ctx.region;
	for (int i = 0; i < 5; ++i) {
		const int regionBorder = 8;
		const int rndValX = random.random(regionBorder, region.getWidthInVoxels() - regionBorder);
		// number should be even
		if (!(rndValX % 2)) {
			continue;
		}

		const int rndValZ = random.random(regionBorder, region.getDepthInVoxels() - regionBorder);
		// TODO: use a noise map to get the position
		glm::ivec3 pos(region.getLowerX() + rndValX, -1, region.getLowerZ() + rndValZ);
		const int y = findFloor(ctx, pos.x, pos.z);
		const int height = random.random(10, 14);
		const int trunkHeight = random.random(5, 9);
		if (y < 0) {
			continue;
		}

		pos.y = y;

		if (!biomManager.hasTrees(pos)) {
			continue;
		}

		const int maxSize = 14;
		const int size = random.random(12, maxSize);
		const int trunkWidth = 1;
		const TreeType treeType = (TreeType)random.random(0, int(TreeType::MAX) - 1);
		addTree(ctx, pos, treeType, trunkHeight, trunkWidth, size, size, height, random);
	}
}

void TreeGenerator::addTree(TerrainContext& ctx, const glm::ivec3& pos, TreeType type, int trunkHeight, int trunkWidth, int width, int depth, int height, core::Random& random) {
	if (type == TreeType::LSYSTEM) {
		// TODO: select leave type via rule
		const VoxelType leavesType = random.random(Leaves1, Leaves10);
		const Voxel leavesVoxel = createVoxel(leavesType);
		LSystemContext lsystemCtx;
		// TODO: improve rule
		lsystemCtx.axiom = "AY[xYA]AY[XYA]AY";
		lsystemCtx.productionRules.emplace('A', lsystemCtx.axiom);
		lsystemCtx.voxels.emplace('A', leavesVoxel);
		lsystemCtx.generations = 2;
		lsystemCtx.start = pos;
		LSystemGenerator::generate(ctx, lsystemCtx, random);
		return;
	}

	int top = (int) pos.y + trunkHeight;
	if (type == TreeType::PINE || type == TreeType::FIR) {
		height *= 2;
		depth *= 2;
		width *= 2;
		top += height;
	}

	static constexpr Voxel voxel = createVoxel(Wood1);
	for (int y = pos.y; y < top; ++y) {
		const int trunkWidthY = trunkWidth + std::max(0, 2 - (y - pos.y));
		for (int x = pos.x - trunkWidthY; x < pos.x + trunkWidthY; ++x) {
			for (int z = pos.z - trunkWidthY; z < pos.z + trunkWidthY; ++z) {
				if ((x >= pos.x + trunkWidthY || x < pos.x - trunkWidthY)
						&& (z >= pos.z + trunkWidthY || z < pos.z - trunkWidthY)) {
					continue;
				}
				glm::ivec3 finalPos(x, y, z);
				if (y == pos.y) {
					finalPos.y = findFloor(ctx, x, z);
					if (finalPos.y < 0) {
						continue;
					}
					for (int i = finalPos.y + 1; i <= y; ++i) {
						ctx.setVoxel(finalPos.x, i, finalPos.z, voxel);
					}
				}

				ctx.setVoxel(finalPos, voxel);
			}
		}
	}

	const VoxelType leavesType = random.random(Leaves1, Leaves10);
	const Voxel leavesVoxel = createVoxel(leavesType);
	const glm::ivec3 leafesPos(pos.x, top + height / 2, pos.z);
	if (type == TreeType::ELLIPSIS) {
		ShapeGenerator::createEllipse(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CONE) {
		ShapeGenerator::createCone(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::FIR) {
		const int branches = 12; //random.random(5, 8);
		const int stepWidth = 360 / branches;
		int angle = random.random(0, stepWidth);
		double w = 1.0;
		for (int b = 0; b < branches; ++b) {
			glm::ivec3 start = leafesPos;
			glm::ivec3 end = start;
			const double x = glm::cos(double(angle));
			const double z = glm::sin(double(angle));
			const int randomZ = random.random(16, 20);
			end.y -= randomZ;
			end.x -= x * w;
			end.z -= z * w;
			ShapeGenerator::createLine(ctx, start, end, leavesVoxel);
			glm::ivec3 end2 = end;
			end2.y -= 2;
			end2.x -= x * w;
			end2.z -= z * w;
			ShapeGenerator::createLine(ctx, end, end2, leavesVoxel);
			angle += stepWidth;
			w += 1.0 / (double)(b + 1);
			Log::info("w: %f", w);
		}
	} else if (type == TreeType::PINE) {
		const int singleLeaveHeight = 2;
		const int singleStepDelta = 1;
		const int singleStepHeight = singleLeaveHeight + singleStepDelta;
		const int steps = std::max(1, height / singleStepHeight);
		const int stepWidth = width / steps;
		const int stepDepth = depth / steps;
		int currentWidth = 2;
		int currentDepth = 2;
		glm::ivec3 leavesPos(pos.x, top, pos.z);
		for (int i = 0; i < steps; ++i) {
			ShapeGenerator::createDome(ctx, leavesPos, currentWidth, singleLeaveHeight, currentDepth, leavesVoxel);
			leavesPos.y -= singleStepDelta;
			ShapeGenerator::createDome(ctx, leavesPos, currentWidth + 1, singleLeaveHeight, currentDepth + 1, leavesVoxel);
			currentDepth += stepDepth;
			currentWidth += stepWidth;
			leavesPos.y -= singleLeaveHeight;
		}
	} else if (type == TreeType::DOME) {
		ShapeGenerator::createDome(ctx, leafesPos, width, height, depth, leavesVoxel);
	} else if (type == TreeType::CUBE) {
		ShapeGenerator::createCube(ctx, leafesPos, width, height, depth, leavesVoxel);
		// TODO: use CreatePlane
		ShapeGenerator::createCube(ctx, leafesPos, width + 2, height - 2, depth - 2, leavesVoxel);
		ShapeGenerator::createCube(ctx, leafesPos, width - 2, height + 2, depth - 2, leavesVoxel);
		ShapeGenerator::createCube(ctx, leafesPos, width - 2, height - 2, depth + 2, leavesVoxel);
	}
}

}

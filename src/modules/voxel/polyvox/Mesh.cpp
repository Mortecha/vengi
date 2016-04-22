#include "Mesh.h"
#include "CubicSurfaceExtractor.h"
#include "core/Common.h"

namespace voxel {

/// Meshes returned by the surface extractors often have vertices with efficient compressed
/// formats which are hard to interpret directly (see CubicVertex and MarchingCubesVertex).
/// This function creates a new uncompressed mesh containing the much simpler Vertex objects.
Mesh<Vertex> decodeMesh(const Mesh<CubicVertex>& encodedMesh) {
	Mesh<Vertex, typename Mesh<CubicVertex>::IndexType> decodedMesh;

	for (typename Mesh<CubicVertex>::IndexType ct = 0; ct < encodedMesh.getNoOfVertices(); ct++) {
		decodedMesh.addVertex(decodeVertex(encodedMesh.getVertex(ct)));
	}

	core_assert_msg(encodedMesh.getNoOfIndices() % 3 == 0, "The number of indices must always be a multiple of three.");
	for (uint32_t ct = 0; ct < encodedMesh.getNoOfIndices(); ct += 3) {
		decodedMesh.addTriangle(encodedMesh.getIndex(ct), encodedMesh.getIndex(ct + 1), encodedMesh.getIndex(ct + 2));
	}

	decodedMesh.setOffset(encodedMesh.getOffset());

	return decodedMesh;
}


}

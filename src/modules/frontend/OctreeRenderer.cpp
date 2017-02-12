/**
 * @file
 */

#include "OctreeRenderer.h"
#include "voxel/MaterialColor.h"
#include "video/ScopedPolygonMode.h"

const std::string MaxDepthBufferUniformName = "u_cascades";

namespace frontend {

OctreeRenderer::RenderOctreeNode::RenderOctreeNode(const video::Shader& shader) {
	for (uint32_t z = 0; z < 2; z++) {
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++) {
				_children[x][y][z] = nullptr;
			}
		}
	}

	_vertexBuffer = _vb.create();
	_indexBuffer = _vb.create(nullptr, 0, video::VertexBufferType::IndexBuffer);

	const int locationPos = shader.enableVertexAttributeArray("a_pos");
	const video::Attribute& posAttrib = getPositionVertexAttribute(_vertexBuffer, locationPos, shader.getAttributeComponents(locationPos));
	_vb.addAttribute(posAttrib);

	const int locationInfo = shader.enableVertexAttributeArray("a_info");
	const video::Attribute& infoAttrib = getInfoVertexAttribute(_vertexBuffer, locationInfo, shader.getAttributeComponents(locationInfo));
	_vb.addAttribute(infoAttrib);
}

OctreeRenderer::RenderOctreeNode::~RenderOctreeNode() {
	for (uint32_t z = 0; z < 2; z++) {
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++) {
				delete _children[x][y][z];
				_children[x][y][z] = nullptr;
			}
		}
	}
	_vb.shutdown();
}

void OctreeRenderer::processOctreeNodeStructure(voxel::OctreeNode* octreeNode, RenderOctreeNode* node) {
	if (octreeNode->_nodeOrChildrenLastChanged <= node->_nodeAndChildrenLastSynced) {
		return;
	}

	if (octreeNode->_propertiesLastChanged > node->_propertiesLastSynced) {
		Log::debug("Resynced properties at %u", node->_propertiesLastSynced);
		node->_renderThisNode = octreeNode->renderThisNode();
		node->_propertiesLastSynced = octreeNode->_octree->time();
	}

	if (octreeNode->_meshLastChanged > node->_meshLastSynced) {
		const voxel::Mesh* mesh = octreeNode->getMesh();
		// TODO: handle water mesh properly
		const voxel::Mesh* waterMesh = octreeNode->getWaterMesh();
		if (mesh != nullptr) {
			glm::ivec3 mins(std::numeric_limits<int>::max());
			glm::ivec3 maxs(std::numeric_limits<int>::min());

			for (auto& v : mesh->getVertexVector()) {
				mins = glm::min(mins, v.position);
				maxs = glm::max(maxs, v.position);
			}
			for (auto& v : waterMesh->getVertexVector()) {
				mins = glm::min(mins, v.position);
				maxs = glm::max(maxs, v.position);
			}

			node->_aabb = core::AABB<float>(mins, maxs);
			node->_vb.update(node->_vertexBuffer, mesh->getVertexVector());
			node->_vb.update(node->_indexBuffer, mesh->getIndexVector());
		}

		node->_meshLastSynced = octreeNode->_octree->time();
		Log::debug("Resynced mesh at %u", node->_meshLastSynced);
	}

	if (octreeNode->_structureLastChanged > node->_structureLastSynced) {
		for (uint32_t z = 0; z < 2; z++) {
			for (uint32_t y = 0; y < 2; y++) {
				for (uint32_t x = 0; x < 2; x++) {
					if (octreeNode->_children[x][y][z] != voxel::Octree::InvalidNodeIndex) {
						if (node->_children[x][y][z] == nullptr) {
							// TODO: pool this
							node->_children[x][y][z] = new RenderOctreeNode(_worldShader);
						}
					} else {
						delete node->_children[x][y][z];
						node->_children[x][y][z] = nullptr;
					}
				}
			}
		}

		node->_structureLastSynced = octreeNode->_octree->time();
		Log::debug("Resynced structure at %u", node->_structureLastSynced);
	}

	octreeNode->visitExistingChildren([=] (uint8_t x, uint8_t y, uint8_t z, voxel::OctreeNode* c) {
		processOctreeNodeStructure(c, node->_children[x][y][z]);
	});
	node->_nodeAndChildrenLastSynced = octreeNode->_octree->time();
}

void OctreeRenderer::renderOctreeNode(const video::Camera& camera, RenderOctreeNode* renderNode) {
	const int numIndices = renderNode->_vb.elements(renderNode->_indexBuffer, 1, sizeof(voxel::IndexType));
	if (numIndices > 0 && renderNode->_renderThisNode) {
		if (camera.isVisible(renderNode->_aabb)) {
			renderNode->_vb.bind();
			video::drawElements<voxel::IndexType>(video::Primitive::Triangles, numIndices);
		}
	}

	// TODO: if we rendered the parent, why on earth should we render the children?
	for (uint32_t z = 0; z < 2; z++) {
		for (uint32_t y = 0; y < 2; y++) {
			for (uint32_t x = 0; x < 2; x++) {
				if (renderNode->_children[x][y][z] == nullptr) {
					continue;
				}
				renderOctreeNode(camera, renderNode->_children[x][y][z]);
			}
		}
	}
}

void OctreeRenderer::render(const video::Camera& camera) {
	core_trace_scoped(OctreeRendererRender);
	voxel::OctreeNode* rootNode = _volume->rootNode();
	if (rootNode != nullptr) {
		processOctreeNodeStructure(rootNode, _rootNode);
	}

	core_trace_gl_scoped(OctreeRendererTraverseOctreeTree);

	video::enable(video::State::DepthTest);
	video::depthFunc(video::CompareFunc::LessEqual);
	video::enable(video::State::CullFace);
	video::enable(video::State::DepthMask);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);

	const std::vector<glm::mat4>& cascades = _shadow.cascades();
	const std::vector<float>& distances = _shadow.distances();
	video::disable(video::State::Blend);
	// put shadow acne into the dark
	video::cullFace(video::Face::Front);
	const float shadowBiasSlope = 2;
	const float shadowBias = 0.09f;
	const float shadowRangeZ = camera.farPlane() * 3.0f;
	const glm::vec2 offset(shadowBiasSlope, (shadowBias / shadowRangeZ) * (1 << 24));
	const video::ScopedPolygonMode scopedPolygonMode(video::PolygonMode::Solid, offset);

	_depthBuffer.bind();
	for (int i = 0; i < maxDepthBuffers; ++i) {
		_depthBuffer.bindTexture(i);
		video::ScopedShader scoped(_shadowMapShader);
		_shadowMapShader.setLightviewprojection(cascades[i]);
		_shadowMapShader.setModel(glm::mat4());
		renderOctreeNode(camera, _rootNode);
	}
	_depthBuffer.unbind();
	video::cullFace(video::Face::Back);
	video::enable(video::State::Blend);
	_colorTexture.bind(video::TextureUnit::Zero);
	video::clearColor(_clearColor);
	video::clear(video::ClearFlag::Color | video::ClearFlag::Depth);
	video::bindTexture(video::TextureUnit::One, _depthBuffer);
	video::ScopedShader scoped(_worldShader);
	_worldShader.setMaterialblock(_materialBlock);
	_worldShader.setViewdistance(camera.farPlane());
	_worldShader.setLightdir(_shadow.sunDirection());
	_worldShader.setFogcolor(_clearColor);
	_worldShader.setTexture(video::TextureUnit::Zero);
	_worldShader.setDiffuseColor(_diffuseColor);
	_worldShader.setAmbientColor(_ambientColor);
	_worldShader.setFogrange(_fogRange);
	_worldShader.setViewprojection(camera.viewProjectionMatrix());
	_worldShader.setShadowmap(video::TextureUnit::One);
	_worldShader.setDepthsize(glm::vec2(_depthBuffer.dimension()));
	_worldShader.setModel(glm::mat4());
	_worldShader.setCascades(cascades);
	_worldShader.setDistances(distances);
	renderOctreeNode(camera, _rootNode);

	_colorTexture.unbind();
}

bool OctreeRenderer::init(voxel::PagedVolume* volume, const voxel::Region& region, int baseNodeSize) {
	if (!_worldShader.setup()) {
		return false;
	}
	if (!_worldInstancedShader.setup()) {
		return false;
	}
	if (!_shadowMapInstancedShader.setup()) {
		return false;
	}
	if (!_waterShader.setup()) {
		return false;
	}
	if (!_shadowMapShader.setup()) {
		return false;
	}
	if (!_shadowMapRenderShader.setup()) {
		return false;
	}

	_rootNode = new RenderOctreeNode(_worldShader);
	_volume = new voxel::OctreeVolume(volume, region, baseNodeSize);
	_colorTexture.init();

	const glm::ivec2& fullscreenQuadIndices = _shadowMapDebugBuffer.createFullscreenTexturedQuad(true);
	video::Attribute attributePos;
	attributePos.bufferIndex = fullscreenQuadIndices.x;
	attributePos.index = _shadowMapRenderShader.getLocationPos();
	attributePos.size = _shadowMapRenderShader.getComponentsPos();
	_shadowMapDebugBuffer.addAttribute(attributePos);

	video::Attribute attributeTexcoord;
	attributeTexcoord.bufferIndex = fullscreenQuadIndices.y;
	attributeTexcoord.index = _shadowMapRenderShader.getLocationTexcoord();
	attributeTexcoord.size = _shadowMapRenderShader.getComponentsTexcoord();
	_shadowMapDebugBuffer.addAttribute(attributeTexcoord);

	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	const glm::ivec2 smSize(core::Var::getSafe(cfg::ClientShadowMapSize)->intVal());
	if (!_depthBuffer.init(smSize, video::DepthBufferMode::DEPTH_CMP, maxDepthBuffers)) {
		return false;
	}

	const int shaderMaterialColorsArraySize = SDL_arraysize(shader::Materialblock::Data::materialcolor);
	const int materialColorsArraySize = voxel::getMaterialColors().size();
	if (shaderMaterialColorsArraySize != materialColorsArraySize) {
		Log::error("Shader parameters and material colors don't match in their size: %i - %i",
				shaderMaterialColorsArraySize, materialColorsArraySize);
		return false;
	}

	shader::Materialblock::Data materialBlock;
	memcpy(materialBlock.materialcolor, &voxel::getMaterialColors().front(), sizeof(materialBlock.materialcolor));
	_materialBlock.create(materialBlock);

	if (!_shadow.init()) {
		return false;
	}

	return true;
}

void OctreeRenderer::update(long dt, const video::Camera& camera) {
	if (_volume == nullptr) {
		return;
	}
	const int maxDepthBuffers = _worldShader.getUniformArraySize(MaxDepthBufferUniformName);
	_shadow.calculateShadowData(camera, true, maxDepthBuffers, _depthBuffer.dimension());
	_volume->update(dt, camera.position(), 1.0f);
}

void OctreeRenderer::shutdown() {
	_shadowMapDebugBuffer.shutdown();
	_shadowMapRenderShader.shutdown();
	_shadowMapInstancedShader.shutdown();
	_worldShader.shutdown();
	_worldInstancedShader.shutdown();
	_waterShader.shutdown();
	_shadowMapShader.shutdown();
	_depthBuffer.shutdown();
	_materialBlock.shutdown();
	_colorTexture.shutdown();
	delete _rootNode;
	_rootNode = nullptr;
	delete _volume;
	_volume = nullptr;
}

}

#pragma once
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <unordered_map>
#include <vector>
#include "../../Core/Systems/SystemSubscribed.h"
#include "../../RenderCore/Camera.h"
#include "../../RenderCore/MaterialAsset.h"
#include "../../RenderCore/MeshAsset.h"
#include "../../RenderCore/Shader.h"
#include "../../RenderCore/Components/RenderableComponent.h"
#include "../Components/TransformComponent.h"
#include "../MultiDrawIndirectStructures.h"

class MultiDrawIndirectRenderingSystem
    : public SystemSubscribed<MultiDrawIndirectRenderingSystem, TransformComponent, RenderableComponent>
{
private:
	Shader& _shader;
	Camera& _camera;
	GLuint* _screenWidth;
	GLuint* _screenHeight;

	GLuint _combinedVAO;
	GLuint _combinedVBO;
	GLuint _combinedEBO;
	GLuint _instanceTransformSSBO;
	GLuint _indirectBuffer;

	std::vector<RenderBatch> _renderBatches;
	std::vector<DrawElementsIndirectCommand> _drawCommands;
	std::vector<InstanceTransform> _allInstanceTransforms;
	CombinedGeometry _combinedGeometry;

	std::unordered_map<BatchKey, size_t> _batchMap;

	static constexpr size_t INITIAL_BATCH_CAPACITY = 1024;
	static constexpr size_t INITIAL_INSTANCE_CAPACITY = 10000;

	bool _buffersInitialized;
	bool _geometryNeedsUpdate;
	bool _instanceDataNeedsUpdate;

public:
	MultiDrawIndirectRenderingSystem(Shader& shader, Camera& camera, GLuint* width, GLuint* height)
	    : _shader(shader), _camera(camera), _screenWidth(width), _screenHeight(height), _combinedVAO(0), _combinedVBO(0),
	      _combinedEBO(0), _instanceTransformSSBO(0), _indirectBuffer(0), _buffersInitialized(false),
	      _geometryNeedsUpdate(true), _instanceDataNeedsUpdate(true)
	{
		InitializeBuffers();
	}

	~MultiDrawIndirectRenderingSystem()
	{
		CleanupBuffers();
	}

	void ProcessEntity(Entity entity, GeneralManager& gm, float deltaTime) override;
	void Update(float deltaTime, GeneralManager& gm, const std::vector<Entity>& entities) override;

private:
	void InitializeBuffers();
	void CleanupBuffers();

	void UpdateCombinedGeometry();
	void UpdateInstanceData();
	void UpdateIndirectCommands();

	void UpdateGeometryBuffers();
	void UpdateInstanceBuffer();
	void UpdateIndirectBuffer();

	void RenderAllBatches();
	void SetupUniforms();
	void BindMaterialTextures(MaterialAsset* material);

	size_t GetOrCreateBatch(const BatchKey& key);
	void ClearFrameData();
};
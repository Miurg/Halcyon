#pragma once
#include <glad/glad.h>
#include <glm/gtc/type_ptr.hpp>
#include <map>
#include <unordered_map>
#include <vector>
#include "../../Core/Systems/SystemSubscribed.h"
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
	Shader* _shader = nullptr;
	GLuint _screenWidth = 0;
	GLuint _screenHeight = 0;

	GLuint _combinedVAO = 0;
	GLuint _combinedVBO = 0;
	GLuint _combinedEBO = 0;
	GLuint _instanceTransformSSBO = 0;
	GLuint _indirectBuffer = 0;

	std::vector<RenderBatch> _renderBatches;
	std::vector<DrawElementsIndirectCommand> _drawCommands;
	std::vector<InstanceTransform> _allInstanceTransforms;
	CombinedGeometry _combinedGeometry;

	std::unordered_map<BatchKey, size_t> _batchMap;

	static constexpr size_t INITIAL_BATCH_CAPACITY = 1024;
	static constexpr size_t INITIAL_INSTANCE_CAPACITY = 10000;

	bool _buffersInitialized = false;
	bool _geometryNeedsUpdate = true;
	bool _instanceDataNeedsUpdate = true;

public:
	MultiDrawIndirectRenderingSystem()
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

	void RenderAllBatches(GeneralManager& gm);
	void SetupUniforms(GeneralManager& gm);
	void BindMaterialTextures(MaterialAsset* material);

	size_t GetOrCreateBatch(const BatchKey& key);
	void ClearFrameData();
};
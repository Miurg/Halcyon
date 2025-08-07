#include "MultiDrawIndirectRenderingSystem.h"

#include <algorithm>
#include <iostream>
#include <unordered_set>

#include "../MultiDrawIndirectStructures.h"

void MultiDrawIndirectRenderingSystem::InitializeBuffers()
{
	glGenVertexArrays(1, &_combinedVAO);
	glBindVertexArray(_combinedVAO);

	glGenBuffers(1, &_combinedVBO);
	glGenBuffers(1, &_combinedEBO);

	glBindBuffer(GL_ARRAY_BUFFER, _combinedVBO);
	glBufferData(GL_ARRAY_BUFFER, MultiDrawConstants::MAX_VERTICES * MultiDrawConstants::VERTEX_SIZE * sizeof(GLfloat),
	             nullptr, GL_DYNAMIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _combinedEBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, MultiDrawConstants::MAX_INDICES * sizeof(GLuint), nullptr, GL_DYNAMIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glGenBuffers(1, &_instanceTransformSSBO);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _instanceTransformSSBO);
	glBufferData(GL_SHADER_STORAGE_BUFFER, MultiDrawConstants::MAX_INSTANCES * sizeof(InstanceTransform), nullptr,
	             GL_DYNAMIC_DRAW);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, _instanceTransformSSBO);

	glGenBuffers(1, &_indirectBuffer);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuffer);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, MultiDrawConstants::MAX_BATCHES * sizeof(DrawElementsIndirectCommand), nullptr,
	             GL_DYNAMIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	_buffersInitialized = true;
	_renderBatches.reserve(INITIAL_BATCH_CAPACITY);
	std::cout << "MultiDrawIndirect buffers initialized successfully" << std::endl;
}

void MultiDrawIndirectRenderingSystem::CleanupBuffers()
{
	if (_buffersInitialized)
	{
		glDeleteVertexArrays(1, &_combinedVAO);
		glDeleteBuffers(1, &_combinedVBO);
		glDeleteBuffers(1, &_combinedEBO);
		glDeleteBuffers(1, &_instanceTransformSSBO);
		glDeleteBuffers(1, &_indirectBuffer);
		_buffersInitialized = false;
	}
}

void MultiDrawIndirectRenderingSystem::ProcessEntity(Entity entity, ComponentManager& cm, float deltaTime)
{
	try
	{
		TransformComponent* transform = cm.GetComponent<TransformComponent>(entity);
		RenderableComponent* renderable = cm.GetComponent<RenderableComponent>(entity);

		size_t batchIndex;
		if (renderable->batchIndex == UINT32_MAX)
		{
			BatchKey key(renderable->Mesh, renderable->Material);
			renderable->batchIndex = GetOrCreateBatch(key);
		}
		batchIndex = renderable->batchIndex;

		auto& batch = _renderBatches[batchIndex];
		uint32_t& idx = batch.entityToInstanceIndex[entity];
		if (idx != -1)
		{
			if (transform->GetDirty())
			{
				batch.instanceTransforms[idx] =
				    InstanceTransform(transform->GetPosition(), transform->GetRotation(), transform->GetScale());
				transform->ClearDirty();
			}
		}
		else
		{

			if (batch.instanceTransforms.size() == batch.instanceTransforms.capacity())
			{
				batch.instanceTransforms.reserve(batch.instanceTransforms.capacity() * 2);
			}
			size_t newInstanceIndex = batch.instanceTransforms.size();
			idx = static_cast<uint32_t>(newInstanceIndex);
			batch.instanceTransforms.emplace_back(transform->GetPosition(), transform->GetRotation(),
			                                      transform->GetScale());
			transform->ClearDirty();
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::MULTIDRAW_RENDER_SYSTEM::Entity " << entity << ": " << e.what() << std::endl;
	}
}

void MultiDrawIndirectRenderingSystem::Update(float deltaTime, ComponentManager& cm,
                                              const std::vector<Entity>& entities)
{
	try
	{
		System::Update(deltaTime, cm, entities);

		if (_geometryNeedsUpdate)
		{
			UpdateCombinedGeometry();
			UpdateGeometryBuffers();
			_geometryNeedsUpdate = false;
		}

		bool hasInstances = false;
		for (const auto& batch : _renderBatches)
		{
			if (!batch.instanceTransforms.empty())
			{
				hasInstances = true;
				break;
			}
		}

		if (hasInstances)
		{
			UpdateInstanceData();
			UpdateInstanceBuffer();

			UpdateIndirectCommands();
			UpdateIndirectBuffer();

			RenderAllBatches();
		}

		ClearFrameData();
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::MULTIDRAW_RENDER_SYSTEM::" << e.what() << std::endl;
	}
}

void MultiDrawIndirectRenderingSystem::UpdateCombinedGeometry()
{
	_combinedGeometry.Clear();

	size_t estimatedVertices = 0;
	size_t estimatedIndices = 0;

	for (const auto& batch : _renderBatches)
	{
		if (batch.mesh && batch.mesh->IsInitialized())
		{
			estimatedVertices += batch.mesh->GetVertexCount();
			estimatedIndices += batch.mesh->GetIndexCount();
		}
	}

	_combinedGeometry.vertices.reserve(estimatedVertices * MultiDrawConstants::VERTEX_SIZE);
	_combinedGeometry.indices.reserve(estimatedIndices);

	GLuint currentVertexOffset = 0;
	GLuint currentIndexOffset = 0;

	for (auto& batch : _renderBatches)
	{
		if (!batch.mesh || !batch.mesh->IsInitialized())
		{
			continue;
		}

		// Store offsets for this batch
		batch.vertexOffset = currentVertexOffset;
		batch.indexOffset = currentIndexOffset;
		batch.indexCount = batch.mesh->GetIndexCount();

		const auto& vertices = batch.mesh->GetVertices();
		for (const auto& vertex : vertices)
		{
			_combinedGeometry.vertices.insert(
			    _combinedGeometry.vertices.end(),
			    {vertex.Position.x, vertex.Position.y, vertex.Position.z, vertex.TexCoord.x, vertex.TexCoord.y});
		}

		const auto& indices = batch.mesh->GetIndices();
		for (GLuint index : indices)
		{
			_combinedGeometry.indices.push_back(index + currentVertexOffset);
		}

		currentVertexOffset += batch.mesh->GetVertexCount();
		currentIndexOffset += batch.mesh->GetIndexCount();
	}

	_combinedGeometry.totalVertexCount = currentVertexOffset;
	_combinedGeometry.totalIndexCount = currentIndexOffset;

	std::cout << "Combined geometry updated: " << _combinedGeometry.totalVertexCount << " vertices, "
	          << _combinedGeometry.totalIndexCount << " indices" << std::endl;
}

void MultiDrawIndirectRenderingSystem::UpdateInstanceData()
{
	size_t totalInstances = 0;
	for (const auto& batch : _renderBatches)
	{
		totalInstances += batch.instanceTransforms.size();
	}
	_allInstanceTransforms.clear();
	_allInstanceTransforms.reserve(totalInstances);

	GLuint currentInstanceOffset = 0;
	for (auto& batch : _renderBatches)
	{
		batch.instanceOffset = currentInstanceOffset;
		_allInstanceTransforms.insert(_allInstanceTransforms.end(), batch.instanceTransforms.begin(),
		                              batch.instanceTransforms.end());
		currentInstanceOffset += static_cast<GLuint>(batch.instanceTransforms.size());
	}
}

void MultiDrawIndirectRenderingSystem::UpdateIndirectCommands()
{
	_drawCommands.reserve(_renderBatches.size());

	for (const auto& batch : _renderBatches)
	{
		if (batch.instanceTransforms.empty())
		{
			continue;
		}

		DrawElementsIndirectCommand command;
		command.count = batch.indexCount;
		command.instanceCount = static_cast<GLuint>(batch.instanceTransforms.size());
		command.firstIndex = batch.indexOffset;
		command.baseVertex = 0; // Always 0, since the indices are already adjusted
		command.baseInstance = batch.instanceOffset;

		_drawCommands.push_back(command);
	}
}

void MultiDrawIndirectRenderingSystem::UpdateGeometryBuffers()
{
	if (_combinedGeometry.vertices.empty() || _combinedGeometry.indices.empty())
	{
		return;
	}

	glBindBuffer(GL_ARRAY_BUFFER, _combinedVBO);
	glBufferSubData(GL_ARRAY_BUFFER, 0, _combinedGeometry.vertices.size() * sizeof(GLfloat),
	                _combinedGeometry.vertices.data());

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _combinedEBO);
	glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, _combinedGeometry.indices.size() * sizeof(GLuint),
	                _combinedGeometry.indices.data());

	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void MultiDrawIndirectRenderingSystem::UpdateInstanceBuffer()
{
	if (_allInstanceTransforms.empty())
	{
		return;
	}

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, _instanceTransformSSBO);
	glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, _allInstanceTransforms.size() * sizeof(InstanceTransform),
	                _allInstanceTransforms.data());
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
}

void MultiDrawIndirectRenderingSystem::UpdateIndirectBuffer()
{
	if (_drawCommands.empty())
	{
		return;
	}

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, _indirectBuffer);
	glBufferSubData(GL_DRAW_INDIRECT_BUFFER, 0, _drawCommands.size() * sizeof(DrawElementsIndirectCommand),
	                _drawCommands.data());
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
}

void MultiDrawIndirectRenderingSystem::RenderAllBatches()
{
	if (_drawCommands.empty())
	{
		return;
	}

	_shader.BindShader();

	SetupUniforms();

	glBindVertexArray(_combinedVAO);

	MaterialAsset* currentMaterial = nullptr;
	size_t validCommandIndex = 0;

	for (size_t batchIndex = 0; batchIndex < _renderBatches.size(); ++batchIndex)
	{
		const auto& batch = _renderBatches[batchIndex];

		if (batch.instanceTransforms.empty())
		{
			continue;
		}

		if (batch.material != currentMaterial)
		{
			if (currentMaterial)
			{
				currentMaterial->UnbindTextures();
			}
			BindMaterialTextures(batch.material);
			currentMaterial = batch.material;
		}

		const auto& command = _drawCommands[validCommandIndex];

		GLint baseInstanceLoc = glGetUniformLocation(_shader.ShaderID, "baseInstance");
		if (baseInstanceLoc != -1)
		{
			glUniform1i(baseInstanceLoc, static_cast<GLint>(command.baseInstance));
		}

		glDrawElementsInstancedBaseVertex(GL_TRIANGLES, command.count, GL_UNSIGNED_INT,
		                                  (void*)(command.firstIndex * sizeof(GLuint)), command.instanceCount,
		                                  command.baseVertex);

		validCommandIndex++;
	}

	if (currentMaterial)
	{
		currentMaterial->UnbindTextures();
	}

	glBindVertexArray(0);
	_shader.UnbindShader();

	// std::cout << "Rendered " << validCommandIndex << " batches with "
	//           << _allInstanceMatrices.size() << " instances" << std::endl;
}

void MultiDrawIndirectRenderingSystem::SetupUniforms()
{
	glm::mat4 view = _camera.GetViewMatrix();
	glUniformMatrix4fv(glGetUniformLocation(_shader.ShaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));

	glm::mat4 projection =
	    glm::perspective(glm::radians(_camera.Fov), (GLfloat)*_screenWidth / (GLfloat)*_screenHeight, 0.1f, 1000.0f);
	glUniformMatrix4fv(glGetUniformLocation(_shader.ShaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
}

void MultiDrawIndirectRenderingSystem::BindMaterialTextures(MaterialAsset* material)
{
	if (!material) return;

	material->BindTextures();

	for (size_t i = 0; i < material->GetTextureCount(); ++i)
	{
		std::string uniformName = "ourTexture" + std::to_string(i + 1);
		glUniform1i(glGetUniformLocation(_shader.ShaderID, uniformName.c_str()), static_cast<GLint>(i));
	}

	GLint hasTexture1Loc = glGetUniformLocation(_shader.ShaderID, "hasTexture1");
	GLint hasTexture2Loc = glGetUniformLocation(_shader.ShaderID, "hasTexture2");

	if (hasTexture1Loc != -1)
	{
		glUniform1i(hasTexture1Loc, material->GetTextureCount() > 0 ? 1 : 0);
	}
	if (hasTexture2Loc != -1)
	{
		glUniform1i(hasTexture2Loc, material->GetTextureCount() > 1 ? 1 : 0);
	}
}

size_t MultiDrawIndirectRenderingSystem::GetOrCreateBatch(const BatchKey& key)
{
	auto it = _batchMap.find(key);
	if (it != _batchMap.end())
	{
		return it->second;
	}
	std::cout << "Creating new batch for mesh " << key.mesh->GetName() << " and material " << key.material->GetName()
	          << std::endl;

	size_t newIndex = _renderBatches.size();
	_renderBatches.emplace_back();
	_renderBatches[newIndex].mesh = key.mesh;
	_renderBatches[newIndex].material = key.material;
	_renderBatches.back().instanceTransforms.reserve(1000);

	_batchMap[key] = newIndex;
	_geometryNeedsUpdate = true;

	return newIndex;
}

void MultiDrawIndirectRenderingSystem::ClearFrameData()
{
	_allInstanceTransforms.clear();
	_drawCommands.clear();
}
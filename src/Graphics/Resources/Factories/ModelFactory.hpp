#pragma once

#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"
#include "../../../Core/Entitys/EntityManager.hpp"
#include "../Managers/BufferManager.hpp"
#include "../Managers/DescriptorManager.hpp"
#include "../Components/BindlessTextureDSetComponent.hpp"
#include "../../../Core/GeneralManager.hpp"
#include "GltfLoader.hpp"

class ModelFactory
{
public:
	static Entity createEntityHierarchy(int parentEntity, tinygltf::Model& model, GeneralManager& gm, int offset,
	                                  BufferManager& bManager, int nodeIndex);
	static Entity loadModel(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bManager,
	                        BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
	                        GeneralManager& gm);
};
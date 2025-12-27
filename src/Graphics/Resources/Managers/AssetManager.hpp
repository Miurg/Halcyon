#pragma once 

#include "VertexIndexBuffer.hpp"
#include <vector>
#include <unordered_map>
#include "../Components/MeshInfoComponent.hpp"
#include "../../VulkanConst.hpp"

class AssetManager
{
	public:
	AssetManager(VulkanDevice& vulkanDevice);
	~AssetManager();
	MeshInfoComponent createMesh(const char path[MAX_PATH_LEN]);
	bool isMeshLoaded();
	MeshInfoComponent addMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh);
	std::vector<VertexIndexBuffer> meshes;
	std::unordered_map<std::string, MeshInfoComponent> meshPaths;

private:
	VulkanDevice& vulkanDevice;
};
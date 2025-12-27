#include "AssetManager.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stdexcept>

AssetManager::AssetManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) {}

AssetManager::~AssetManager() {}

MeshInfoComponent AssetManager::createMesh(const char path[MAX_PATH_LEN])
{
	if (isMeshLoaded(path))
	{
		return meshPaths[std::string(path)];
	}
	for (int i = 0; i < meshes.size(); i++)
	{
		if (sizeof(meshes[i].vertices) < MAX_SIZE_OF_VERTEX_INDEX_BUFFER)
		{
			MeshInfoComponent info = addMeshFromFile(path, meshes[i]);
			info.bufferIndex = static_cast<uint32_t>(i);
			meshes[i].createVertexBuffer(vulkanDevice);
			meshes[i].createIndexBuffer(vulkanDevice);
			meshPaths[std::string(path)] = info;
			return info;
		}
	}

	meshes.push_back(VertexIndexBuffer());
	MeshInfoComponent info = addMeshFromFile(path, meshes.back());
	info.bufferIndex = static_cast<uint32_t>(meshes.size() - 1);
	meshes.back().createVertexBuffer(vulkanDevice);
	meshes.back().createIndexBuffer(vulkanDevice);
	meshPaths[std::string(path)] = info;
	return info;
}

bool AssetManager::isMeshLoaded(const char path[MAX_PATH_LEN])
{
	std::string pathStr(path);
	return meshPaths.find(pathStr) != meshPaths.end();
}

MeshInfoComponent AssetManager::addMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh)
{
	uint32_t firstIndex = static_cast<uint32_t>(mesh.indices.size());
	int32_t vertexOffset = static_cast<int32_t>(mesh.vertices.size());
	tinyobj::attrib_t attrib;
	std::vector<tinyobj::shape_t> shapes;
	std::vector<tinyobj::material_t> materials;
	std::string warn, err;

	std::unordered_map<Vertex, uint32_t> uniqueVertices{};

	if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, path))
	{
		throw std::runtime_error(warn + err);
	}

	for (const auto& shape : shapes)
	{
		for (const auto& index : shape.mesh.indices)
		{
			Vertex vertex{};

			vertex.pos = {attrib.vertices[3 * index.vertex_index + 0], attrib.vertices[3 * index.vertex_index + 1],
			              attrib.vertices[3 * index.vertex_index + 2]};

			vertex.texCoord = {attrib.texcoords[2 * index.texcoord_index + 0],
			                   1.0f - attrib.texcoords[2 * index.texcoord_index + 1]};

			vertex.color = {1.0f, 1.0f, 1.0f};

			if (uniqueVertices.count(vertex) == 0)
			{
				uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size()) - vertexOffset;
				mesh.vertices.push_back(vertex);
			}

			mesh.indices.push_back(uniqueVertices[vertex]);
		}
	}

	uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size()) - firstIndex;

	 MeshInfoComponent info;
	 info.indexCount = indexCount;
	 info.indexOffset = firstIndex;
	 info.vertexOffset = vertexOffset;
	 memcpy(info.path, path, MAX_PATH_LEN);
	 return info;
}

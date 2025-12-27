#include "AssetManager.hpp"
#include "AssetManager.hpp"
#define TINYOBJLOADER_IMPLEMENTATION
#include <tiny_obj_loader.h>
#include <stdexcept>

AssetManager::AssetManager(VulkanDevice& vulkanDevice) : vulkanDevice(vulkanDevice) {}

AssetManager::~AssetManager() {}

MeshInfoComponent AssetManager::createMesh(const char path[MAX_PATH_LEN])
{
	meshes.push_back(VertexIndexBuffer());
	MeshInfoComponent info = addMeshFromFile(path, meshes[0]);
	info.bufferIndex = 0;
	meshes[0].createVertexBuffer(vulkanDevice);
	meshes[0].createIndexBuffer(vulkanDevice);
	return info;
}

bool AssetManager::isMeshLoaded()
{
	return !meshes.empty();
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
				uniqueVertices[vertex] = static_cast<uint32_t>(mesh.vertices.size());
				mesh.vertices.push_back(vertex);
			}

			mesh.indices.push_back(uniqueVertices[vertex]);
		}
	}

	std::cout << "Added mesh from " << path << ". Total: " << mesh.vertices.size() << " vertices, " << mesh.indices.size()
	          << " indices." << std::endl;
	uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size()) - firstIndex;

	 MeshInfoComponent info;
	 info.indexCount = indexCount;
	 info.indexOffset = firstIndex;
	 info.vertexOffset = vertexOffset;
	 memcpy(info.path, path, MAX_PATH_LEN);
	 return info;
}

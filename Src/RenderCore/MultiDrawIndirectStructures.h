#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>
#include <unordered_map>
#include <vector>

class MeshAsset;
class MaterialAsset;

struct InstanceTransform
{
	glm::vec4 Position; // w component is not used
	glm::quat Rotation;
	glm::vec4 Scale; // w component is not used

	InstanceTransform()
	    : Position(0.0f, 0.0f, 0.0f, 0.0f), Rotation(1.0f, 0.0f, 0.0f, 0.0f), Scale(1.0f, 1.0f, 1.0f, 0.0f)
	{
	}
	InstanceTransform(const glm::vec3& pos, const glm::quat& rot, const glm::vec3& scl)
	    : Position(pos.x, pos.y, pos.z, 0.0f), Rotation(rot), Scale(scl.x, scl.y, scl.z, 0.0f)
	{
	}
};

struct DrawElementsIndirectCommand
{
	GLuint count;
	GLuint instanceCount;
	GLuint firstIndex;
	GLuint baseVertex;
	GLuint baseInstance;
};

struct RenderBatch
{
	MeshAsset* mesh;
	MaterialAsset* material;
	std::vector<InstanceTransform> instanceTransforms;
	std::vector<uint32_t> entityToInstanceIndex;
	GLuint instanceOffset;
	GLuint instanceCount;
	GLuint vertexOffset;
	GLuint indexOffset;
	GLuint indexCount;

	RenderBatch()
	    : mesh(nullptr), material(nullptr), instanceOffset(0), instanceCount(0), vertexOffset(0), indexOffset(0),
	      indexCount(0)
	{
		entityToInstanceIndex.resize(1 << 24, -1);
	}
};

struct CombinedGeometry
{
	std::vector<GLfloat> vertices;
	std::vector<GLuint> indices;
	GLuint totalVertexCount;
	GLuint totalIndexCount;

	CombinedGeometry() : totalVertexCount(0), totalIndexCount(0) {}

	void Clear()
	{
		vertices.clear();
		indices.clear();
		totalVertexCount = 0;
		totalIndexCount = 0;
	}
};

struct BatchKey
{
	MeshAsset* mesh;
	MaterialAsset* material;

	BatchKey(MeshAsset* m, MaterialAsset* mat) : mesh(m), material(mat) {}
	BatchKey(const BatchKey&) = default;
	BatchKey& operator=(const BatchKey&) = default;

	bool operator==(const BatchKey& other) const
	{
		return mesh == other.mesh && material == other.material;
	}
};
namespace std
{
template <>
struct hash<BatchKey>
{
	size_t operator()(const BatchKey& key) const
	{
		return hash<void*>{}(key.mesh) ^ (hash<void*>{}(key.material) << 1);
	}
};

} // namespace std

namespace MultiDrawConstants
{
constexpr size_t MAX_INSTANCES = 1000000;
constexpr size_t MAX_BATCHES = 1000;
constexpr size_t MAX_VERTICES = 10000000;
constexpr size_t MAX_INDICES = 30000000;
constexpr size_t VERTEX_SIZE = 5;
} // namespace MultiDrawConstants
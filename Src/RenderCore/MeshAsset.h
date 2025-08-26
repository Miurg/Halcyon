#pragma once
#include <glad/glad.h>
#include <glm/glm.hpp>
#include <string>
#include <vector>

struct Vertex
{
	glm::vec3 Position;
	glm::vec2 TexCoord;
};

class MeshAsset
{
private:
	GLuint _indexCount;
	std::string _name;
	bool _initialized;

	std::vector<Vertex> _vertices;
	std::vector<GLuint> _indices;

public:
	MeshAsset(const std::string& meshName) : _name(meshName), _initialized(false) {}

	void CreateFromData(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
	{
		_indexCount = static_cast<GLuint>(indices.size());

		_vertices = vertices;
		_indices = indices;

		_initialized = true;

		std::cout << "Created mesh: " << _name << " with " << vertices.size() << " vertices and " << indices.size()
		          << " indices" << std::endl;
	}

	GLuint GetIndexCount() const
	{
		return _indexCount;
	}
	const std::string& GetName() const
	{
		return _name;
	}
	bool IsInitialized() const
	{
		return _initialized;
	}

	const std::vector<Vertex>& GetVertices() const
	{
		return _vertices;
	}

	const std::vector<GLuint>& GetIndices() const
	{
		return _indices;
	}

	GLuint GetVertexCount() const
	{
		return static_cast<GLuint>(_vertices.size());
	}
};
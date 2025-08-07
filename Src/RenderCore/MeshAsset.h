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
	GLuint _vao, _vbo, _ebo;
	GLuint _indexCount;
	std::string _name;
	bool _initialized;
	
	std::vector<Vertex> _vertices;
	std::vector<GLuint> _indices;

public:
	MeshAsset(const std::string& meshName) : _name(meshName), _initialized(false) {}

	~MeshAsset()
	{
		if (_initialized)
		{
			glDeleteVertexArrays(1, &_vao);
			glDeleteBuffers(1, &_vbo);
			glDeleteBuffers(1, &_ebo);
		}
	}

	void CreateFromData(const std::vector<Vertex>& vertices, const std::vector<GLuint>& indices)
	{
		if (_initialized)
		{
			glDeleteVertexArrays(1, &_vao);
			glDeleteBuffers(1, &_vbo);
			glDeleteBuffers(1, &_ebo);
		}

		_indexCount = static_cast<GLuint>(indices.size());
		
		_vertices = vertices;
		_indices = indices;

		glGenVertexArrays(1, &_vao);
		glGenBuffers(1, &_vbo);
		glGenBuffers(1, &_ebo);

		glBindVertexArray(_vao);

		glBindBuffer(GL_ARRAY_BUFFER, _vbo);
		glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(Vertex), vertices.data(), GL_STATIC_DRAW);

		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, _ebo);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

		// Position attribute
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)0);
		glEnableVertexAttribArray(0);

		// Texture coordinate attribute
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, TexCoord));
		glEnableVertexAttribArray(1);

		glBindVertexArray(0);
		_initialized = true;

		// std::cout << "Created mesh: " << name << " with " << vertices.size() << " vertices and " << indices.size() << "
		// indices" << std::endl;
	}

	void BindMesh() const
	{
		if (_initialized)
		{
			glBindVertexArray(_vao);
		}
	}

	void UnbindMesh() const
	{
		glBindVertexArray(0);
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
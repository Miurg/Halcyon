#include "PrimitiveMeshFactory.h"

#include <array>
#include <cmath>

namespace
{
const std::array<Vertex, 24> kCubeVertices = {{//(Z = +0.5)
                                               {{-0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                                               {{0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},
                                               {{-0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},

                                               //(Z = -0.5)
                                               {{0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                                               {{-0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
                                               {{-0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},

                                               //(X = -0.5)
                                               {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                                               {{-0.5f, -0.5f, 0.5f}, {1.0f, 0.0f}},
                                               {{-0.5f, 0.5f, 0.5f}, {1.0f, 1.0f}},
                                               {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}},

                                               //(X = +0.5)
                                               {{0.5f, -0.5f, 0.5f}, {0.0f, 0.0f}},
                                               {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {0.0f, 1.0f}},

                                               //(Y = -0.5)
                                               {{-0.5f, -0.5f, -0.5f}, {0.0f, 0.0f}},
                                               {{0.5f, -0.5f, -0.5f}, {1.0f, 0.0f}},
                                               {{0.5f, -0.5f, 0.5f}, {1.0f, 1.0f}},
                                               {{-0.5f, -0.5f, 0.5f}, {0.0f, 1.0f}},

                                               //(Y = +0.5)
                                               {{-0.5f, 0.5f, 0.5f}, {0.0f, 0.0f}},
                                               {{0.5f, 0.5f, 0.5f}, {1.0f, 0.0f}},
                                               {{0.5f, 0.5f, -0.5f}, {1.0f, 1.0f}},
                                               {{-0.5f, 0.5f, -0.5f}, {0.0f, 1.0f}}}};

const std::array<GLuint, 36> kCubeIndices = {{0,  2,  1,  0,  3,  2,  4,  6,  5,  4,  7,  6,  8,  10, 9,  8,  11, 10,
                                              12, 14, 13, 12, 15, 14, 16, 18, 17, 16, 19, 18, 20, 22, 21, 20, 23, 22}};
} // namespace

MeshAsset* PrimitiveMeshFactory::CreateCube(AssetManager* assetManager, const std::string& name)
{
	if (assetManager->HasMesh(name))
	{
		return assetManager->GetMesh(name);
	}

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	GenerateCubeData(vertices, indices);

	MeshAsset* mesh = assetManager->CreateMesh(name);
	mesh->CreateFromData(vertices, indices);
	return mesh;
}

void PrimitiveMeshFactory::GenerateCubeData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices)
{
	vertices.assign(kCubeVertices.begin(), kCubeVertices.end());
	indices.assign(kCubeIndices.begin(), kCubeIndices.end());
}

MeshAsset* PrimitiveMeshFactory::CreateSphere(AssetManager* assetManager, const std::string& name, int segments,
                                              int rings)
{
	if (assetManager->HasMesh(name))
	{
		return assetManager->GetMesh(name);
	}

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	GenerateSphereData(vertices, indices, segments, rings);

	MeshAsset* mesh = assetManager->CreateMesh(name);
	mesh->CreateFromData(vertices, indices);
	return mesh;
}

void PrimitiveMeshFactory::GenerateSphereData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, int segments,
                                              int rings)
{
	vertices.clear();
	indices.clear();

	const float PI = 3.14159265359f;

	for (int ring = 0; ring <= rings; ++ring)
	{
		float phi = static_cast<float>(ring) * PI / static_cast<float>(rings);
		float y = cos(phi);
		float radius = sin(phi);

		for (int segment = 0; segment <= segments; ++segment)
		{
			float theta = static_cast<float>(segment) * 2.0f * PI / static_cast<float>(segments);
			float x = radius * cos(theta);
			float z = radius * sin(theta);

			glm::vec3 position(x * 0.5f, y * 0.5f, z * 0.5f);

			float u = static_cast<float>(segment) / static_cast<float>(segments);
			float v = static_cast<float>(ring) / static_cast<float>(rings);

			vertices.push_back({position, {u, v}});
		}
	}

	for (int ring = 0; ring < rings; ++ring)
	{
		for (int segment = 0; segment < segments; ++segment)
		{
			int current = ring * (segments + 1) + segment;
			int next = current + segments + 1;

			indices.push_back(current);
			indices.push_back(next);
			indices.push_back(current + 1);

			indices.push_back(current + 1);
			indices.push_back(next);
			indices.push_back(next + 1);
		}
	}
}

MeshAsset* PrimitiveMeshFactory::CreatePlane(AssetManager* assetManager, const std::string& name, float width,
                                             float height)
{
	if (assetManager->HasMesh(name))
	{
		return assetManager->GetMesh(name);
	}

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	GeneratePlaneData(vertices, indices, width, height);

	MeshAsset* mesh = assetManager->CreateMesh(name);
	mesh->CreateFromData(vertices, indices);
	return mesh;
}

void PrimitiveMeshFactory::GeneratePlaneData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, float width,
                                             float height)
{
	float halfWidth = width * 0.5f;
	float halfHeight = height * 0.5f;

	vertices = {{{-halfWidth, 0.0f, -halfHeight}, {0.0f, 0.0f}},
	            {{halfWidth, 0.0f, -halfHeight}, {1.0f, 0.0f}},
	            {{halfWidth, 0.0f, halfHeight}, {1.0f, 1.0f}},
	            {{-halfWidth, 0.0f, halfHeight}, {0.0f, 1.0f}}};

	indices = {0, 1, 2, 0, 2, 3};
}

MeshAsset* PrimitiveMeshFactory::CreateCylinder(AssetManager* assetManager, const std::string& name, float radius,
                                                float height, int segments)
{
	if (assetManager->HasMesh(name))
	{
		return assetManager->GetMesh(name);
	}

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	GenerateCylinderData(vertices, indices, radius, height, segments);

	MeshAsset* mesh = assetManager->CreateMesh(name);
	mesh->CreateFromData(vertices, indices);
	return mesh;
}

void PrimitiveMeshFactory::GenerateCylinderData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices,
                                                float radius, float height, int segments)
{
	vertices.clear();
	indices.clear();

	const float PI = 3.14159265359f;
	float halfHeight = height * 0.5f;

	vertices.push_back({{0.0f, -halfHeight, 0.0f}, {0.5f, 0.5f}});
	vertices.push_back({{0.0f, halfHeight, 0.0f}, {0.5f, 0.5f}});

	for (int i = 0; i <= segments; ++i)
	{
		float angle = static_cast<float>(i) * 2.0f * PI / static_cast<float>(segments);
		float x = radius * cos(angle);
		float z = radius * sin(angle);
		float u = static_cast<float>(i) / static_cast<float>(segments);

		vertices.push_back({{x, -halfHeight, z}, {u, 0.0f}});

		vertices.push_back({{x, halfHeight, z}, {u, 1.0f}});
	}

	for (int i = 0; i < segments; ++i)
	{
		int base = 2 + i * 2;

		indices.push_back(base);
		indices.push_back(base + 2);
		indices.push_back(base + 1);

		indices.push_back(base + 1);
		indices.push_back(base + 2);
		indices.push_back(base + 3);
	}

	for (int i = 0; i < segments; ++i)
	{
		int base = 2 + i * 2;
		int next = 2 + ((i + 1) % segments) * 2;

		indices.push_back(0);
		indices.push_back(next);
		indices.push_back(base);

		indices.push_back(1);
		indices.push_back(base + 1);
		indices.push_back(next + 1);
	}
}

MeshAsset* PrimitiveMeshFactory::CreateCone(AssetManager* assetManager, const std::string& name, float radius,
                                            float height, int segments)
{
	if (assetManager->HasMesh(name))
	{
		return assetManager->GetMesh(name);
	}

	std::vector<Vertex> vertices;
	std::vector<GLuint> indices;
	GenerateConeData(vertices, indices, radius, height, segments);

	MeshAsset* mesh = assetManager->CreateMesh(name);
	mesh->CreateFromData(vertices, indices);
	return mesh;
}

void PrimitiveMeshFactory::GenerateConeData(std::vector<Vertex>& vertices, std::vector<GLuint>& indices, float radius,
                                            float height, int segments)
{
	vertices.clear();
	indices.clear();

	const float PI = 3.14159265359f;
	float halfHeight = height * 0.5f;

	vertices.push_back({{0.0f, halfHeight, 0.0f}, {0.5f, 1.0f}});

	vertices.push_back({{0.0f, -halfHeight, 0.0f}, {0.5f, 0.5f}});

	for (int i = 0; i <= segments; ++i)
	{
		float angle = static_cast<float>(i) * 2.0f * PI / static_cast<float>(segments);
		float x = radius * cos(angle);
		float z = radius * sin(angle);
		float u = static_cast<float>(i) / static_cast<float>(segments);

		vertices.push_back({{x, -halfHeight, z}, {u, 0.0f}});
	}

	for (int i = 0; i < segments; ++i)
	{
		int base = 2 + i;
		int next = 2 + ((i + 1) % segments);

		indices.push_back(0);
		indices.push_back(base);
		indices.push_back(next);
	}

	for (int i = 0; i < segments; ++i)
	{
		int base = 2 + i;
		int next = 2 + ((i + 1) % segments);

		indices.push_back(1);
		indices.push_back(next);
		indices.push_back(base);
	}
}
#pragma once

#include <array>
#include <vulkan/vulkan_raii.hpp>
#include <glm/glm.hpp>

struct Vertex
{
	glm::vec3 pos;
	glm::vec3 color;
	glm::vec3 normal;
	glm::vec2 texCoord;

	static vk::VertexInputBindingDescription getBindingDescription()
	{
		return {0, sizeof(Vertex), vk::VertexInputRate::eVertex};
	}

	bool operator==(const Vertex& other) const
	{
		return pos == other.pos && color == other.color && normal == other.normal && texCoord == other.texCoord;
	}

	static std::array<vk::VertexInputAttributeDescription, 4> getAttributeDescriptions()
	{
		return 
		{
			vk::VertexInputAttributeDescription(0, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, pos)),
		    vk::VertexInputAttributeDescription(1, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, color)),
		    vk::VertexInputAttributeDescription(2, 0, vk::Format::eR32G32B32Sfloat, offsetof(Vertex, normal)),
		    vk::VertexInputAttributeDescription(3, 0, vk::Format::eR32G32Sfloat, offsetof(Vertex, texCoord))
		};
	}
};

namespace std
{
template <>
struct hash<Vertex>
{
	size_t operator()(Vertex const& vertex) const noexcept
	{
		// 64-bit friendly combine (boost-like)
		auto hashFloat = [](float v) noexcept { return std::hash<uint32_t>()(*reinterpret_cast<uint32_t const*>(&v)); };

		size_t h = hashFloat(vertex.pos.x);
		h ^= hashFloat(vertex.pos.y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= hashFloat(vertex.pos.z) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);

		h ^= hashFloat(vertex.color.x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= hashFloat(vertex.color.y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= hashFloat(vertex.color.z) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);

		h ^= hashFloat(vertex.texCoord.x) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
		h ^= hashFloat(vertex.texCoord.y) + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);

		return h;
	}
};
} // namespace std
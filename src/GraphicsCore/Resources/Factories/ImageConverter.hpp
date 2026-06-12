#pragma once

#include "../Managers/VertexIndexBuffer.hpp"
#include "../Managers/Texture.hpp"
#include "../Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "../../VulkanDevice.hpp"
#include "../Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>

class ImageConverter
{
public:
	static std::vector<unsigned char> convertToRGBA(const tinygltf::Image& img);
};
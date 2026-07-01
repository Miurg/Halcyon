#pragma once

#include "GraphicsCore/Resources/Managers/VertexIndexBuffer.hpp"
#include "GraphicsCore/Resources/Managers/Texture.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include <vk_mem_alloc.h>
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Resources/Managers/PrimitivesInfo.hpp"
#include <tiny_gltf.h>

class ImageConverter
{
public:
	static std::vector<unsigned char> convertToRGBA(const tinygltf::Image& img);
};
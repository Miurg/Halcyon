#include "GltfLoader.hpp"
#include "ImageConverter.hpp"

int GltfLoader::loadModelFromFile(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bManager,
                                  BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
                                  tinygltf::Model model, TextureManager& tManager, ModelManager& mManager)
{
	VertexIndexBuffer& vertexIndexBuffer = mManager.vertexIndexBuffers[vertexIndexBInt];
	int32_t globalVertexOffset = static_cast<int32_t>(vertexIndexBuffer.vertices.size()); // To adjust indices

	MaterialMaps materialMaps = materialsParser(model, tManager, dSetComponent, dManager, bManager);

	std::vector<MeshInfo> loadedMeshes;
	loadedMeshes.resize(model.meshes.size());
	for (int i = 0; i < model.meshes.size(); ++i)
	{
		std::vector<PrimitivesInfo> loadedPrimitives;
		auto newPrims = primitiveParser(model.meshes[i], vertexIndexBuffer, model, globalVertexOffset, materialMaps);
		loadedMeshes[i].primitives = newPrims;
		loadedMeshes[i].vertexIndexBufferID = vertexIndexBInt;
		model.meshes[i].name.copy(loadedMeshes[i].path, sizeof(loadedMeshes[i].path) - 1); // Copy name
	}

	int offsetForInSystemMesh = static_cast<int>(mManager.meshes.size());
	mManager.meshes.insert(mManager.meshes.end(), loadedMeshes.begin(), loadedMeshes.end());

	return offsetForInSystemMesh;
}

MaterialMaps GltfLoader::materialsParser(tinygltf::Model& model, TextureManager& tManager,
                                         BindlessTextureDSetComponent& dSetComponent, DescriptorManager& dManager,
                                         BufferManager& bManager)
{
	MaterialMaps maps;
	glm::vec4 colorFactor = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white
	int whiteTexture =
	    tManager.isTextureLoaded("sys_default_white")
	        ? tManager.texturePaths["sys_default_white"].id
	        : tManager
	              .generateTextureData("sys_default_white", 1, 1, std::vector<unsigned char>{255, 255, 255, 255}.data(),
	                                   dSetComponent, dManager)
	              .id;

	// Default flat normal map: (128,128,255,255) = tangent-space up (0,0,1), loaded as linear
	int defaultNormalTexture =
	    tManager.isTextureLoaded("sys_default_normal")
	        ? tManager.texturePaths["sys_default_normal"].id
	        : tManager
	              .generateTextureData("sys_default_normal", 1, 1, std::vector<unsigned char>{128, 128, 255, 255}.data(),
	                                   dSetComponent, dManager, vk::Format::eR8G8B8A8Unorm)
	              .id;
	int defaultMRTexture =
	    tManager.isTextureLoaded("sys_default_mr")
	        ? tManager.texturePaths["sys_default_mr"].id
	        : tManager
	              .generateTextureData("sys_default_mr", 1, 1, std::vector<unsigned char>{255, 128, 0, 255}.data(),
	                                   dSetComponent, dManager, vk::Format::eR8G8B8A8Unorm)
	              .id;
	int defaultEmissiveTexture =
	    tManager.isTextureLoaded("sys_default_emissive")
	        ? tManager.texturePaths["sys_default_emissive"].id
	        : tManager
	              .generateTextureData("sys_default_emissive", 1, 1, std::vector<unsigned char>{0, 0, 0, 255}.data(),
	                                   dSetComponent, dManager)
	              .id;
	for (size_t i = 0; i < model.materials.size(); i++)
	{
		MaterialStructure material{};
		// Base color factor
		auto colorIt = model.materials[i].values.find("baseColorFactor");
		if (colorIt != model.materials[i].values.end())
		{
			const auto& colorVal = colorIt->second.ColorFactor();
			colorFactor = {static_cast<float>(colorVal[0]), static_cast<float>(colorVal[1]),
			               static_cast<float>(colorVal[2]), static_cast<float>(colorVal[3])};
		}

		// Base color texture
		auto texIt = model.materials[i].values.find("baseColorTexture");
		if (texIt != model.materials[i].values.end())
		{
			int textureIndex = texIt->second.TextureIndex();
			if (textureIndex >= 0 && textureIndex < model.textures.size())
			{
				const tinygltf::Texture& tex = model.textures[textureIndex];
				int sourceImageIndex = tex.source;
				if (sourceImageIndex >= 0 && sourceImageIndex < model.images.size())
				{
					tinygltf::Image& img = model.images[sourceImageIndex];
					if (!img.image.empty() && img.width > 0 && img.height > 0)
					{
						auto newTex = std::make_shared<TextureData>();
						if (!img.name.empty())
							newTex->name = img.name;
						else if (!img.uri.empty() && img.uri.find("data:") != 0)
							newTex->name = img.uri;
						int indexInSystem =
						    tManager
						        .generateTextureData(newTex->name.c_str(), img.width, img.height,
						                             ImageConverter::convertToRGBA(img).data(), dSetComponent, dManager)
						        .id;
						material.textureIndex = indexInSystem;
					}
				}
			}
		}
		else
		{
			material.textureIndex = whiteTexture;
		}

		// Normal map texture
		auto normTexIt = model.materials[i].additionalValues.find("normalTexture");
		if (normTexIt != model.materials[i].additionalValues.end())
		{
			int textureIndex = normTexIt->second.TextureIndex();
			if (textureIndex >= 0 && textureIndex < model.textures.size())
			{
				const tinygltf::Texture& tex = model.textures[textureIndex];
				int sourceImageIndex = tex.source;
				if (sourceImageIndex >= 0 && sourceImageIndex < model.images.size())
				{
					tinygltf::Image& img = model.images[sourceImageIndex];
					if (!img.image.empty() && img.width > 0 && img.height > 0)
					{
						std::string normalName;
						if (!img.name.empty())
							normalName = "normal_" + img.name;
						else if (!img.uri.empty() && img.uri.find("data:") != 0)
							normalName = "normal_" + img.uri;
						else
							normalName = "normal_mat" + std::to_string(i);

						int indexInSystem =
						    tManager
						        .generateTextureData(normalName.c_str(), img.width, img.height,
						                             ImageConverter::convertToRGBA(img).data(), dSetComponent, dManager,
						                             vk::Format::eR8G8B8A8Unorm) // Linear format for normal maps
						        .id;
						material.normalMapIndex = indexInSystem;
					}
				}
			}
		}
		else
		{
			material.normalMapIndex = defaultNormalTexture;
		}
		// Metallic-Roughness texture (packed: G=Roughness, B=Metallic)
		auto mrTexIt = model.materials[i].values.find("metallicRoughnessTexture");
		if (mrTexIt != model.materials[i].values.end())
		{
			int textureIndex = mrTexIt->second.TextureIndex();
			if (textureIndex >= 0 && textureIndex < (int)model.textures.size())
			{
				const tinygltf::Texture& tex = model.textures[textureIndex];
				int sourceImageIndex = tex.source;
				if (sourceImageIndex >= 0 && sourceImageIndex < (int)model.images.size())
				{
					tinygltf::Image& img = model.images[sourceImageIndex];
					if (!img.image.empty() && img.width > 0 && img.height > 0)
					{
						std::string mrName;
						if (!img.name.empty())
							mrName = "mr_" + img.name;
						else if (!img.uri.empty() && img.uri.find("data:") != 0)
							mrName = "mr_" + img.uri;
						else
							mrName = "mr_mat" + std::to_string(i);

						int indexInSystem =
						    tManager
						        .generateTextureData(mrName.c_str(), img.width, img.height,
						                             ImageConverter::convertToRGBA(img).data(), dSetComponent, dManager,
						                             vk::Format::eR8G8B8A8Unorm) // Linear
						        .id;
						material.metallicRoughnessIndex = indexInSystem;
					}
				}
			}
		}
		else
		{
			material.metallicRoughnessIndex = defaultMRTexture;
		}
		// Emissive texture
		auto emissiveTexIt = model.materials[i].additionalValues.find("emissiveTexture");
		if (emissiveTexIt != model.materials[i].additionalValues.end())
		{
			int textureIndex = emissiveTexIt->second.TextureIndex();
			if (textureIndex >= 0 && textureIndex < (int)model.textures.size())
			{
				const tinygltf::Texture& tex = model.textures[textureIndex];
				int sourceImageIndex = tex.source;
				if (sourceImageIndex >= 0 && sourceImageIndex < (int)model.images.size())
				{
					tinygltf::Image& img = model.images[sourceImageIndex];
					if (!img.image.empty() && img.width > 0 && img.height > 0)
					{
						std::string emissiveName;
						if (!img.name.empty())
							emissiveName = "emissive_" + img.name;
						else if (!img.uri.empty() && img.uri.find("data:") != 0)
							emissiveName = "emissive_" + img.uri;
						else
							emissiveName = "emissive_mat" + std::to_string(i);

						int indexInSystem =
						    tManager
						        .generateTextureData(emissiveName.c_str(), img.width, img.height,
						                             ImageConverter::convertToRGBA(img).data(), dSetComponent, dManager) //sRBG
						        .id;
						material.emissiveIndex = indexInSystem;
					}
				}
			}
		}
		else
		{
			material.emissiveIndex = defaultEmissiveTexture;
		}
		maps.materials.emplace(i, tManager.emplaceMaterials(dSetComponent, material, bManager));
	}
	return maps;
}

std::vector<PrimitivesInfo> GltfLoader::primitiveParser(tinygltf::Mesh& mesh, VertexIndexBuffer& vertexIndexB,
                                                        tinygltf::Model& model, int32_t globalVertexOffset,
                                                        const MaterialMaps& materialMaps)
{
	std::vector<PrimitivesInfo> loadedPrimitives;
	for (const auto& primitive : mesh.primitives)
	{
		uint32_t firstIndex = static_cast<uint32_t>(vertexIndexB.indices.size());
		// Read positions
		const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.at("POSITION")];
		const tinygltf::BufferView& posBufferView = model.bufferViews[posAccessor.bufferView];
		const tinygltf::Buffer& posBuffer = model.buffers[posBufferView.buffer];

		// Read texture coordinates if present
		const tinygltf::Accessor* texAccessor = nullptr;
		const tinygltf::BufferView* texBufferView = nullptr;
		const tinygltf::Buffer* texBuffer = nullptr;
		if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end())
		{
			texAccessor = &model.accessors[primitive.attributes.at("TEXCOORD_0")];
			texBufferView = &model.bufferViews[texAccessor->bufferView];
			texBuffer = &model.buffers[texBufferView->buffer];
		}

		// Read normals if present
		const tinygltf::Accessor* normAccessor = nullptr;
		const tinygltf::BufferView* normBufferView = nullptr;
		const tinygltf::Buffer* normBuffer = nullptr;
		if (primitive.attributes.find("NORMAL") != primitive.attributes.end())
		{
			normAccessor = &model.accessors[primitive.attributes.at("NORMAL")];
			normBufferView = &model.bufferViews[normAccessor->bufferView];
			normBuffer = &model.buffers[normBufferView->buffer];
		}

		// Read tangents if present
		const tinygltf::Accessor* tangAccessor = nullptr;
		const tinygltf::BufferView* tangBufferView = nullptr;
		const tinygltf::Buffer* tangBuffer = nullptr;
		if (primitive.attributes.find("TANGENT") != primitive.attributes.end())
		{
			tangAccessor = &model.accessors[primitive.attributes.at("TANGENT")];
			tangBufferView = &model.bufferViews[tangAccessor->bufferView];
			tangBuffer = &model.buffers[tangBufferView->buffer];
		}

		uint32_t currentLocalOffset = static_cast<uint32_t>(vertexIndexB.vertices.size()) - globalVertexOffset;

		const unsigned char* posDataStart = &posBuffer.data[posBufferView.byteOffset + posAccessor.byteOffset];
		int posByteStride =
		    posAccessor.ByteStride(posBufferView) ? posAccessor.ByteStride(posBufferView) : sizeof(float) * 3;

		const unsigned char* texDataStart = nullptr;
		int texByteStride = 0;
		if (texAccessor)
		{
			texDataStart = &texBuffer->data[texBufferView->byteOffset + texAccessor->byteOffset];
			texByteStride =
			    texAccessor->ByteStride(*texBufferView) ? texAccessor->ByteStride(*texBufferView) : sizeof(float) * 2;
		}

		const unsigned char* normDataStart = nullptr;
		int normByteStride = 0;
		if (normAccessor)
		{
			normDataStart = &normBuffer->data[normBufferView->byteOffset + normAccessor->byteOffset];
			normByteStride =
			    normAccessor->ByteStride(*normBufferView) ? normAccessor->ByteStride(*normBufferView) : sizeof(float) * 3;
		}

		const unsigned char* tangDataStart = nullptr;
		int tangByteStride = 0;
		if (tangAccessor)
		{
			tangDataStart = &tangBuffer->data[tangBufferView->byteOffset + tangAccessor->byteOffset];
			tangByteStride =
			    tangAccessor->ByteStride(*tangBufferView) ? tangAccessor->ByteStride(*tangBufferView) : sizeof(float) * 4;
		}

		// Read vertices
		size_t oldVertexSize = vertexIndexB.vertices.size();
		vertexIndexB.vertices.resize(oldVertexSize + posAccessor.count);
		Vertex* outputVertices = vertexIndexB.vertices.data() + oldVertexSize;

		glm::vec3 maxBound = {0.0f, 0.0f, 0.0f};
		glm::vec3 minBound = {0.0f, 0.0f, 0.0f};
		if (!posAccessor.minValues.empty() && !posAccessor.maxValues.empty())
		{
			minBound = {static_cast<float>(posAccessor.minValues[0]), static_cast<float>(posAccessor.minValues[1]),
			            static_cast<float>(posAccessor.minValues[2])};

			maxBound = {static_cast<float>(posAccessor.maxValues[0]), static_cast<float>(posAccessor.maxValues[1]),
			            static_cast<float>(posAccessor.maxValues[2])};
		}
		for (size_t i = 0; i < posAccessor.count; i++)
		{
			Vertex& v = outputVertices[i];

			const float* pos = reinterpret_cast<const float*>(posDataStart + (i * posByteStride));
			v.pos = {pos[0], pos[1], pos[2]};

			if (normAccessor)
			{
				const float* norm = reinterpret_cast<const float*>(normDataStart + (i * normByteStride));
				v.normal = {norm[0], norm[1], norm[2]};
			}
			else
			{
				v.normal = {0.0f, 1.0f, 0.0f}; // Default normal
			}

			if (texAccessor)
			{
				const float* tex = reinterpret_cast<const float*>(texDataStart + (i * texByteStride));
				v.texCoord = {tex[0], tex[1]};
			}
			else
			{
				v.texCoord = {0.0f, 0.0f};
			}

			if (tangAccessor)
			{
				const float* tang = reinterpret_cast<const float*>(tangDataStart + (i * tangByteStride));
				v.tangent = {tang[0], tang[1], tang[2], tang[3]};
			}
			else
			{
				v.tangent = {1.0f, 0.0f, 0.0f, 1.0f}; // Default tangent
			}

			v.color = {1.0f, 1.0f, 1.0f};
		}

		// Read indices
		const tinygltf::Accessor& indexAccessor = model.accessors[primitive.indices];
		const tinygltf::BufferView& indexBufferView = model.bufferViews[indexAccessor.bufferView];
		const tinygltf::Buffer& indexBuffer = model.buffers[indexBufferView.buffer];
		const unsigned char* indexData = &indexBuffer.data[indexBufferView.byteOffset + indexAccessor.byteOffset];

		size_t oldIndexSize = vertexIndexB.indices.size();
		vertexIndexB.indices.resize(oldIndexSize + indexAccessor.count);
		uint32_t* outputIndices = vertexIndexB.indices.data() + oldIndexSize;

		// Support only unsigned types
		auto pushIndex = [&](uint32_t gltfIndex, size_t i) { outputIndices[i] = currentLocalOffset + gltfIndex; };

		if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT)
		{
			const uint16_t* buf = reinterpret_cast<const uint16_t*>(indexData);
			for (size_t i = 0; i < indexAccessor.count; i++) pushIndex(buf[i], i);
		}
		else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_INT)
		{
			const uint32_t* buf = reinterpret_cast<const uint32_t*>(indexData);
			for (size_t i = 0; i < indexAccessor.count; i++) pushIndex(buf[i], i);
		}
		else if (indexAccessor.componentType == TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE)
		{
			const uint8_t* buf = reinterpret_cast<const uint8_t*>(indexData);
			for (size_t i = 0; i < indexAccessor.count; i++) pushIndex(buf[i], i);
		}
		uint32_t materialIndex = primitive.material;

		uint32_t indexCount = static_cast<uint32_t>(vertexIndexB.indices.size()) - firstIndex;

		PrimitivesInfo result;
		result.indexCount = indexCount;
		result.indexOffset = firstIndex;
		result.vertexOffset = globalVertexOffset;
		result.AABBMax = maxBound;
		result.AABBMin = minBound;

		// Assign material
		auto itBase = materialMaps.materials.find(materialIndex);
		result.materialIndex = (itBase != materialMaps.materials.end()) ? itBase->second : 0;

		loadedPrimitives.push_back(result);
	}
	return loadedPrimitives;
}

std::vector<MeshInfo> GltfLoader::modelParser(tinygltf::Model model)
{
	std::vector<MeshInfo> lol = {};
	return lol;
}

std::shared_ptr<TextureData> GltfLoader::createDefaultWhiteTexture()
{
	auto texture = std::make_shared<TextureData>();

	texture->width = 1;
	texture->height = 1;
	texture->name = "sys_default_white";

	// Single white pixel
	texture->pixels = {255, 255, 255, 255};

	return texture;
}
#include "GltfLoader.hpp"
#include "ImageConverter.hpp"
#include "GraphicsCore/Resources/Factories/TextureFactory.hpp"
#include <filesystem>
#include <algorithm>
#include <stdexcept>
#include <utility>

int GltfLoader::loadModelFromFile(const char path[MAX_PATH_LEN], int vertexIndexBInt, BufferManager& bufferManager,
                                  BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager,
                                  tinygltf::Model& model, TextureManager& textureManager, ModelManager& modelManager,
                                  MaterialManager& materialManager, VulkanDevice& vulkanDevice, VmaAllocator allocator)
{
	MaterialMaps materialMaps = materialsParser(model, textureManager, materialManager, dSetComponent, descriptorManager,
	                                            bufferManager, path, vulkanDevice, allocator);

	std::vector<Vertex> localVertices;
	std::vector<uint32_t> localIndices;

	std::vector<MeshInfo> loadedMeshes;
	loadedMeshes.resize(model.meshes.size());
	for (int i = 0; i < model.meshes.size(); ++i)
	{
		auto newPrims = primitiveParser(model.meshes[i], localVertices, localIndices, model, 0, materialMaps);
		loadedMeshes[i].primitives = newPrims;
		loadedMeshes[i].vertexIndexBufferID = vertexIndexBInt;
		model.meshes[i].name.copy(loadedMeshes[i].path, sizeof(loadedMeshes[i].path) - 1); // Copy name
	}

	GeometryAllocation allocation{};
	allocation.bufferIndex = vertexIndexBInt;
	if (!localVertices.empty())
	{
		auto allocated = modelManager.allocateGeometry(vertexIndexBInt, static_cast<uint32_t>(localVertices.size()),
		                                               static_cast<uint32_t>(localIndices.size()));
		if (!allocated)
		{
			throw std::runtime_error("Out of geometry buffer space while loading model");
		}
		allocation = *allocated;

		for (auto& loadedMesh : loadedMeshes)
		{
			for (auto& primitive : loadedMesh.primitives)
			{
				primitive.vertexOffset += allocation.vertexBase;
				primitive.indexOffset += allocation.indexBase;
			}
		}

		modelManager.uploadVertices(vertexIndexBInt, allocation.vertexBase, localVertices.data(),
		                            static_cast<uint32_t>(localVertices.size()));
		modelManager.uploadIndices(vertexIndexBInt, allocation.indexBase, localIndices.data(),
		                           static_cast<uint32_t>(localIndices.size()));
	}

	std::vector<int> meshSlots;
	meshSlots.reserve(loadedMeshes.size());
	for (auto& loadedMesh : loadedMeshes)
	{
		int slot = modelManager.allocateMeshSlot();
		modelManager.getMesh(slot) = std::move(loadedMesh);
		meshSlots.push_back(slot);
	}

	std::vector<int> ownedMaterials;
	ownedMaterials.reserve(materialMaps.materials.size());
	for (const auto& [gltfIndex, materialSlot] : materialMaps.materials)
		ownedMaterials.push_back(static_cast<int>(materialSlot));

	int modelIndex = modelManager.allocateModelSlot();
	modelManager.getModel(modelIndex).allocation = allocation;
	modelManager.getModel(modelIndex).meshes = std::move(meshSlots);
	modelManager.getModel(modelIndex).textures = std::move(materialMaps.ownedTextures);
	modelManager.getModel(modelIndex).materials = std::move(ownedMaterials);
	modelManager.modelPaths[path] = modelIndex;

	return modelIndex;
}

int GltfLoader::loadMaterialTexture(tinygltf::Model& model, const std::map<std::string, tinygltf::Parameter>& params,
                                    const char* paramName, bool isSrgb, int fallback, const char* filePath,
                                    std::vector<int>& ownedTextures, TextureManager& textureManager,
                                    BindlessTextureDSetComponent& dSetComponent, DescriptorManager& descriptorManager,
                                    VulkanDevice& vulkanDevice, VmaAllocator allocator)
{
	auto paramIt = params.find(paramName);
	if (paramIt == params.end()) return fallback;

	int textureIndex = paramIt->second.TextureIndex();
	if (textureIndex < 0 || textureIndex >= static_cast<int>(model.textures.size())) return fallback;

	const tinygltf::Texture& tex = model.textures[textureIndex];
	int sourceImageIndex = tex.source;
	auto basisuIt = tex.extensions.find("KHR_texture_basisu");
	if (basisuIt != tex.extensions.end() && basisuIt->second.Has("source"))
		sourceImageIndex = basisuIt->second.Get("source").GetNumberAsInt();
	if (sourceImageIndex < 0 || sourceImageIndex >= static_cast<int>(model.images.size())) return fallback;

	tinygltf::Image& img = model.images[sourceImageIndex];
	// External files key by their resolved on-disk path so models can share them;
	// embedded images have no identity outside their model file.
	std::string texName;
	if (!img.uri.empty() && img.uri.find("data:") != 0)
	{
		std::string decodedUri = img.uri;
		tinygltf::URIDecode(img.uri, &decodedUri, nullptr);
		texName = std::filesystem::absolute(std::filesystem::path(filePath).parent_path() / decodedUri)
		              .lexically_normal()
		              .generic_string();
	}
	else
	{
		texName = std::string(filePath) + "#img" + std::to_string(sourceImageIndex);
	}
	// The same bytes under sRGB vs UNORM are two different images.
	texName += isSrgb ? "|srgb" : "|linear";

	if (textureManager.isTextureLoaded(texName.c_str()))
	{
		TextureHandle handle = textureManager.texturePaths[texName];
		textureManager.addTextureRef(handle);
		ownedTextures.push_back(handle.id);
		return handle.id;
	}
	if (img.as_is && img.mimeType == "image/ktx2" && !img.image.empty())
	{
		TextureHandle handle = TextureFactory::generateTextureDataFromKtx(
		    textureManager, vulkanDevice, allocator, texName.c_str(), img.image.data(), img.image.size(), dSetComponent,
		    descriptorManager, isSrgb);
		ownedTextures.push_back(handle.id);
		return handle.id;
	}
	if (!img.image.empty() && img.width > 0 && img.height > 0)
	{
		auto rgbaPixels = ImageConverter::convertToRGBA(img);
		if (!rgbaPixels.empty())
		{
			TextureHandle handle = TextureFactory::generateTextureData(
			    textureManager, vulkanDevice, allocator, texName.c_str(), img.width, img.height, rgbaPixels.data(),
			    dSetComponent, descriptorManager, isSrgb ? vk::Format::eR8G8B8A8Srgb : vk::Format::eR8G8B8A8Unorm);
			ownedTextures.push_back(handle.id);
			return handle.id;
		}
	}
	return fallback;
}

MaterialMaps GltfLoader::materialsParser(tinygltf::Model& model, TextureManager& textureManager,
                                         MaterialManager& materialManager, BindlessTextureDSetComponent& dSetComponent,
                                         DescriptorManager& descriptorManager, BufferManager& bufferManager,
                                         const char* filePath, VulkanDevice& vulkanDevice, VmaAllocator allocator)
{
	MaterialMaps maps;
	glm::vec4 colorFactor = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white
	int whiteTexture = textureManager.isTextureLoaded("sys_default_white")
	                       ? textureManager.texturePaths["sys_default_white"].id
	                       : TextureFactory::generateTextureData(
	                             textureManager, vulkanDevice, allocator, "sys_default_white", 1, 1,
	                             std::vector<unsigned char>{255, 255, 255, 255}.data(), dSetComponent, descriptorManager)
	                             .id;
	// Default flat normal map: (128,128,255,255) = tangent-space up (0,0,1), loaded as linear
	int defaultNormalTexture =
	    textureManager.isTextureLoaded("sys_default_normal")
	        ? textureManager.texturePaths["sys_default_normal"].id
	        : TextureFactory::generateTextureData(textureManager, vulkanDevice, allocator, "sys_default_normal", 1, 1,
	                                              std::vector<unsigned char>{128, 128, 255, 255}.data(), dSetComponent,
	                                              descriptorManager, vk::Format::eR8G8B8A8Unorm)
	              .id;
	int defaultMRTexture =
	    textureManager.isTextureLoaded("sys_default_mr")
	        ? textureManager.texturePaths["sys_default_mr"].id
	        : TextureFactory::generateTextureData(textureManager, vulkanDevice, allocator, "sys_default_mr", 1, 1,
	                                              std::vector<unsigned char>{255, 255, 0, 255}.data(), dSetComponent,
	                                              descriptorManager, vk::Format::eR8G8B8A8Unorm)
	              .id;
	int defaultEmissiveTexture =
	    textureManager.isTextureLoaded("sys_default_emissive")
	        ? textureManager.texturePaths["sys_default_emissive"].id
	        : TextureFactory::generateTextureData(textureManager, vulkanDevice, allocator, "sys_default_emissive", 1, 1,
	                                              std::vector<unsigned char>{255, 255, 255, 255}.data(), dSetComponent,
	                                              descriptorManager)
	              .id;

	MaterialStructure defaultMaterial{};
	defaultMaterial.textureIndex = whiteTexture;
	defaultMaterial.normalMapIndex = defaultNormalTexture;
	defaultMaterial.metallicRoughnessIndex = defaultMRTexture;
	defaultMaterial.emissiveIndex = defaultEmissiveTexture;

	maps.materials.emplace(static_cast<uint32_t>(-1),
	                       materialManager.emplaceMaterial(dSetComponent, defaultMaterial, bufferManager));

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
			material.baseColorFactor = colorFactor;
		}

		material.textureIndex = loadMaterialTexture(model, model.materials[i].values, "baseColorTexture", /*isSrgb*/ true,
		                                            whiteTexture, filePath, maps.ownedTextures, textureManager,
		                                            dSetComponent, descriptorManager, vulkanDevice, allocator);
		material.normalMapIndex = loadMaterialTexture(
		    model, model.materials[i].additionalValues, "normalTexture", /*isSrgb*/ false, defaultNormalTexture, filePath,
		    maps.ownedTextures, textureManager, dSetComponent, descriptorManager, vulkanDevice, allocator);
		// Metallic-Roughness texture (packed: G=Roughness, B=Metallic);
		material.metallicRoughnessIndex = loadMaterialTexture(
		    model, model.materials[i].values, "metallicRoughnessTexture", /*isSrgb*/ false, defaultMRTexture, filePath,
		    maps.ownedTextures, textureManager, dSetComponent, descriptorManager, vulkanDevice, allocator);

		// Metallic-Roughness factors (defaults are 1.0 as per GLTF spec)
		auto roughnessFactorIt = model.materials[i].values.find("roughnessFactor");
		if (roughnessFactorIt != model.materials[i].values.end())
		{
			material.roughnessFactor = static_cast<float>(roughnessFactorIt->second.number_value);
		}

		auto metallicFactorIt = model.materials[i].values.find("metallicFactor");
		if (metallicFactorIt != model.materials[i].values.end())
		{
			material.metallicFactor = static_cast<float>(metallicFactorIt->second.number_value);
		}

		material.emissiveIndex = loadMaterialTexture(
		    model, model.materials[i].additionalValues, "emissiveTexture", /*isSrgb*/ true, defaultEmissiveTexture,
		    filePath, maps.ownedTextures, textureManager, dSetComponent, descriptorManager, vulkanDevice, allocator);
		// Emissive Factor
		bool hasEmissiveTexture = (material.emissiveIndex != defaultEmissiveTexture);
		auto emissiveFactorIt = model.materials[i].additionalValues.find("emissiveFactor");
		if (emissiveFactorIt != model.materials[i].additionalValues.end())
		{
			const auto& factorVal = emissiveFactorIt->second.ColorFactor();
			if (factorVal.size() >= 3)
			{
				material.emissiveFactor = {static_cast<float>(factorVal[0]), static_cast<float>(factorVal[1]),
				                           static_cast<float>(factorVal[2])};
			}
		}

		// Emissive Strength (KHR_materials_emissive_strength)
		auto extIt = model.materials[i].extensions.find("KHR_materials_emissive_strength");
		if (extIt != model.materials[i].extensions.end())
		{
			if (extIt->second.Has("emissiveStrength"))
			{
				material.emissiveStrength = static_cast<float>(extIt->second.Get("emissiveStrength").GetNumberAsDouble());
			}
		}

		// Alpha Mode
		auto alphaModeIt = model.materials[i].additionalValues.find("alphaMode");
		if (alphaModeIt != model.materials[i].additionalValues.end())
		{
			if (alphaModeIt->second.string_value == "MASK")
				material.alphaMode = 1;
			else if (alphaModeIt->second.string_value == "BLEND")
				material.alphaMode = 2;
			else
				material.alphaMode = 0; // OPAQUE
		}

		// Alpha Cutoff
		auto alphaCutoffIt = model.materials[i].additionalValues.find("alphaCutoff");
		if (alphaCutoffIt != model.materials[i].additionalValues.end())
		{
			material.alphaCutoff = static_cast<float>(alphaCutoffIt->second.number_value);
		}

		// Double Sided
		material.doubleSided = model.materials[i].doubleSided ? 1 : 0;

		maps.materials.emplace(static_cast<uint32_t>(i),
		                       materialManager.emplaceMaterial(dSetComponent, material, bufferManager));
	}

	return maps;
}

std::vector<PrimitivesInfo> GltfLoader::primitiveParser(tinygltf::Mesh& mesh, std::vector<Vertex>& outVertices,
                                                        std::vector<uint32_t>& outIndices, tinygltf::Model& model,
                                                        int32_t globalVertexOffset, const MaterialMaps& materialMaps)
{
	std::vector<PrimitivesInfo> loadedPrimitives;
	for (const auto& primitive : mesh.primitives)
	{
		uint32_t firstIndex = static_cast<uint32_t>(outIndices.size());
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

		uint32_t currentLocalOffset = static_cast<uint32_t>(outVertices.size()) - globalVertexOffset;

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
		size_t oldVertexSize = outVertices.size();
		outVertices.resize(oldVertexSize + posAccessor.count);
		Vertex* outputVertices = outVertices.data() + oldVertexSize;

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

		size_t oldIndexSize = outIndices.size();
		outIndices.resize(oldIndexSize + indexAccessor.count);
		uint32_t* outputIndices = outIndices.data() + oldIndexSize;

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

		uint32_t indexCount = static_cast<uint32_t>(outIndices.size()) - firstIndex;

		PrimitivesInfo result;
		result.indexCount = indexCount;
		result.indexOffset = firstIndex;
		result.vertexOffset = globalVertexOffset;
		result.AABBMax = maxBound;
		result.AABBMin = minBound;

		// Assign material
		auto itBase = materialMaps.materials.find(materialIndex);
		if (itBase != materialMaps.materials.end())
		{
			result.materialIndex = itBase->second;
		}
		else
		{
			// Fallback to the default material we created at index -1
			auto itDefault = materialMaps.materials.find(static_cast<uint32_t>(-1));
			result.materialIndex = (itDefault != materialMaps.materials.end()) ? itDefault->second : 0;
		}

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
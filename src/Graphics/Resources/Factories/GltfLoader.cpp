#include "GltfLoader.hpp"
#include "ImageConverter.hpp"

std::vector<PrimitivesInfo> GltfLoader::loadModelFromFile(const char path[MAX_PATH_LEN],
                                                           VertexIndexBuffer& vertexIndexB, BufferManager& bManager,
                                                           BindlessTextureDSetComponent& dSetComponent,
                                                           DescriptorManager& dManager)
{
	int32_t globalVertexOffset = static_cast<int32_t>(vertexIndexB.vertices.size()); // To adjust indices

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	std::string err, warn;
	bool ret = false;

	std::string pathStr = path;
	if (pathStr.size() >= 4 && pathStr.substr(pathStr.size() - 4) == ".glb")
	{
		ret = loader.LoadBinaryFromFile(&model, &err, &warn, path);
	}
	else
	{
		ret = loader.LoadASCIIFromFile(&model, &err, &warn, path);
	}

	if (!err.empty())
	{
		throw std::runtime_error("glTF error: " + err);
	}
	if (!ret)
	{
		throw std::runtime_error("Failed to load glTF model");
	}

	std::vector<PrimitivesInfo> loadedPrimitives;

	for (const auto& gltfMesh : model.meshes)
	{
		auto newPrims =
		    primitiveParser(const_cast<tinygltf::Mesh&>(gltfMesh), vertexIndexB, model, globalVertexOffset);
		loadedPrimitives.insert(loadedPrimitives.end(), newPrims.begin(), newPrims.end());
	}
	std::unordered_map<uint32_t, uint32_t> replacementMapTextures = materialsParser(
	    model, bManager, dSetComponent, dManager); // Map from index in glTF to texture index in our system

	int whiteTexture = bManager.isTextureLoaded("sys_default_white")
						   ? bManager.texturePaths["sys_default_white"]
						   : bManager.generateTextureData(
								 "sys_default_white", 1, 1,
	                                                      std::vector<unsigned char>{255, 255, 255, 255}.data(),
	                                                      dSetComponent, dManager);
	for (int i = 0; i < loadedPrimitives.size(); i++)
	{
		uint32_t materialIndex = loadedPrimitives[i].textureIndex;
		auto it = replacementMapTextures.find(materialIndex);
		if (it != replacementMapTextures.end())
		{
			loadedPrimitives[i].textureIndex = it->second;
		}
		else
		{
			loadedPrimitives[i].textureIndex = whiteTexture; // No texture
		}
	}

	return loadedPrimitives;
}

std::unordered_map<uint32_t, uint32_t> GltfLoader::materialsParser(tinygltf::Model& model, BufferManager& bManager,
                                                                   BindlessTextureDSetComponent& dSetComponent,
                                                                   DescriptorManager& dManager)
{
	std::unordered_map<uint32_t, uint32_t>
	    replacementMapTextures; // Map from index in glTF to texture index in our system
	glm::vec4 colorFactor = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white

	for (size_t i = 0; i < model.materials.size(); i++)
	{
		// Base color factor
		auto colorIt = model.materials[i].values.find("baseColorFactor");
		if (colorIt != model.materials[i].values.end())
		{
			const auto& colorVal = colorIt->second.ColorFactor();
			colorFactor = {static_cast<float>(colorVal[0]), static_cast<float>(colorVal[1]),
			               static_cast<float>(colorVal[2]), static_cast<float>(colorVal[3])};
		}
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
					// Load image
					tinygltf::Image& img = model.images[sourceImageIndex];
					if (!img.image.empty() && img.width > 0 && img.height > 0)
					{
						auto newTex = std::make_shared<TextureData>();
						// Determine texture name
						if (!img.name.empty())
						{
							newTex->name = img.name;
						}
						else if (!img.uri.empty() && img.uri.find("data:") != 0)
						{
							newTex->name = img.uri;
						}
						int indexInSystem = bManager.generateTextureData(newTex->name.c_str(), img.width, img.height,
						                                               ImageConverter::convertToRGBA(img).data(),
						                                               dSetComponent, dManager);
						replacementMapTextures.emplace(i, indexInSystem);
					}
				}
			}
		}
	}
	return replacementMapTextures;
}

std::vector<PrimitivesInfo> GltfLoader::primitiveParser(tinygltf::Mesh& mesh, VertexIndexBuffer& vertexIndexB,
                                                        tinygltf::Model& model, int32_t globalVertexOffset)
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

		// Read vertices
		size_t oldVertexSize = vertexIndexB.vertices.size();
		vertexIndexB.vertices.resize(oldVertexSize + posAccessor.count);
		Vertex* outputVertices = vertexIndexB.vertices.data() + oldVertexSize;
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

		glm::vec4 colorFactor = {1.0f, 1.0f, 1.0f, 1.0f}; // Default white
		uint32_t materialIndex = primitive.material;

		uint32_t indexCount = static_cast<uint32_t>(vertexIndexB.indices.size()) - firstIndex;

		PrimitivesInfo result;
		result.indexCount = indexCount;
		result.indexOffset = firstIndex;
		result.vertexOffset = globalVertexOffset;
		result.textureIndex = materialIndex;
		result.baseColorFactor = colorFactor;

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
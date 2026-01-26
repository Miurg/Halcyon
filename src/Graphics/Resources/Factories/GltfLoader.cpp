#include "GltfLoader.hpp"
#include "ImageConverter.hpp"
std::tuple<PrimitivesInfo, std::vector<unsigned char>, int, int>
GltfLoader::loadMeshFromFile(const char path[MAX_PATH_LEN], VertexIndexBuffer& mesh)
{
	uint32_t firstIndex = static_cast<uint32_t>(mesh.indices.size());
	int32_t globalVertexOffset = static_cast<int32_t>(mesh.vertices.size()); // To adjust indices

	tinygltf::Model model;
	tinygltf::TinyGLTF loader;
	tinygltf::Image img;
	std::string err, warn;
	bool ret = false;
	// loader.SetImageLoader([](tinygltf::Image*, const int, std::string*, std::string*, int, int, const unsigned char*,
	//                          int, void*) { return true; },
	//                       nullptr); // Off image loading
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

	for (const auto& gltfMesh : model.meshes)
	{
		for (const auto& primitive : gltfMesh.primitives)
		{
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

			uint32_t currentLocalOffset = static_cast<uint32_t>(mesh.vertices.size()) - globalVertexOffset;

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
				normByteStride = normAccessor->ByteStride(*normBufferView) ? normAccessor->ByteStride(*normBufferView)
				                                                           : sizeof(float) * 3;
			}
			// Read vertices
			size_t oldVertexSize = mesh.vertices.size();
			mesh.vertices.resize(oldVertexSize + posAccessor.count);
			Vertex* outputVertices = mesh.vertices.data() + oldVertexSize;
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
			size_t oldIndexSize = mesh.indices.size();
			mesh.indices.resize(oldIndexSize + indexAccessor.count);
			uint32_t* outputIndices = mesh.indices.data() + oldIndexSize;

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
			if (primitive.material >= 0)
			{
				const tinygltf::Material& mat = model.materials[primitive.material];

				auto texIt = mat.values.find("baseColorTexture");
				if (texIt != mat.values.end())
				{
					int textureIndex = texIt->second.TextureIndex();
					if (textureIndex >= 0 && textureIndex < model.textures.size())
					{
						const tinygltf::Texture& tex = model.textures[textureIndex];
						if (tex.source >= 0 && tex.source < model.images.size())
						{
							img = model.images[tex.source];
						}
					}
				}
			}
		}
	}

	uint32_t indexCount = static_cast<uint32_t>(mesh.indices.size()) - firstIndex;

	PrimitivesInfo info;
	info.indexCount = indexCount;
	info.indexOffset = firstIndex;
	info.vertexOffset = globalVertexOffset;

	std::vector<unsigned char> rgbaData = ImageConverter::convertToRGBA(img);

	if (img.image.empty() || img.width <= 0 || img.height <= 0)
	{
		return {info, {}, 0, 0};
	}

	return {info, std::move(rgbaData), img.width, img.height};
}

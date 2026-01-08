#include "GameInit.hpp"
#include "../Graphics/Resources/Components/MeshInfoComponent.hpp"
#include "../Graphics/Resources/Managers/BufferManager.hpp"
#include "../Graphics/Resources/Components/TextureInfoComponent.hpp"
#include "../CoreInit.hpp"
#include "../Graphics/GraphicsContexts.hpp"
#include "../Graphics/Components/BufferManagerComponent.hpp"
#include <vulkan/vulkan_raii.hpp>

void GameInit::gameInitStart(GeneralManager& gm)
{
	BufferManager* bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	int j = 0;
	int k = 0;
	for (int i = 0; i < 100; i++)
	{
		MeshInfoComponent meshInfo;
		int numberTexture;
		if (i > 50)
		{
			meshInfo = bufferManager->createMesh("assets/models/BlenderMonkey.obj");
			numberTexture = bufferManager->generateTextureData("assets/textures/texture.jpg", vk::Format::eR8G8B8A8Srgb,
			                                                   vk::ImageAspectFlagBits::eColor);
		}
		else
		{
			meshInfo = bufferManager->createMesh("assets/models/viking_room.obj");
			numberTexture = bufferManager->generateTextureData("assets/textures/viking_room.png",
			                                                   vk::Format::eR8G8B8A8Srgb, vk::ImageAspectFlagBits::eColor);
		}

		Entity gameObjectEntity1 = gm.createEntity();
		gm.addComponent<TransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f);
		gm.addComponent<MeshInfoComponent>(gameObjectEntity1, meshInfo);
		gm.addComponent<TextureInfoComponent>(gameObjectEntity1, numberTexture);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);
		gm.subscribeEntity<BufferUpdateSystem>(gameObjectEntity1);

		k++;
		if ((i + 1) % 10 == 0)
		{
			j++;
			k = 0;
		}
	}
}
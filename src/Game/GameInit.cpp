#include "GameInit.hpp"
#include "../Graphics/Resources/Components/MeshInfoComponent.hpp"
#include "../Graphics/Resources/Managers/BufferManager.hpp"
#include "../Graphics/Resources/Components/TextureInfoComponent.hpp"
#include "../CoreInit.hpp"
#include "../Graphics/GraphicsContexts.hpp"
#include "../Graphics/Components/BufferManagerComponent.hpp"
#include "../Graphics/Components/DescriptorManagerComponent.hpp"
#include <vulkan/vulkan_raii.hpp>

void GameInit::gameInitStart(GeneralManager& gm)
{
	BufferManager* bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	BindlessTextureDSetComponent* dSetComponent = gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	int j = 0;
	int k = 0;
	for (int i = 0; i < 10000; i++)
	{
		Entity gameObjectEntity1 = gm.createEntity();
		MeshInfoComponent meshInfo;
		int numberTexture;
		if (j%2 == 0)
		{
			gm.addComponent<TransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f, 0.0f, 0.0f, 0.0f);
			meshInfo.mesh = bufferManager->createMesh("assets/models/BlenderMonkey.obj");
			numberTexture = bufferManager->generateTextureData("assets/textures/texture.jpg", vk::Format::eR8G8B8A8Srgb,
			                                                   vk::ImageAspectFlagBits::eColor, *dSetComponent, *dManager);
		}
		else
		{
			gm.addComponent<TransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f, -90.0f, 0.0f, 0.0f);
			meshInfo.mesh = bufferManager->createMesh("assets/models/viking_room.obj");
			numberTexture = bufferManager->generateTextureData("assets/textures/viking_room.png", vk::Format::eR8G8B8A8Srgb,
			                                       vk::ImageAspectFlagBits::eColor, *dSetComponent, *dManager);
		}



		gm.addComponent<MeshInfoComponent>(gameObjectEntity1, meshInfo);
		gm.addComponent<TextureInfoComponent>(gameObjectEntity1, numberTexture);
		gm.subscribeEntity<RotationSystem>(gameObjectEntity1);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);

		k++;
		if ((i + 1) % 100 == 0)
		{
			j++;
			k = 0;
		}
	}
}
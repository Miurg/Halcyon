#include "SpawnSystem.hpp"
#include <iostream>
#include "../../Core/GeneralManager.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Graphics/Components/GlobalTransformComponent.hpp"
#include "../../Graphics/Resources/Components/MeshInfoComponent.hpp"
#include "../../Graphics/Resources/Managers/BufferManager.hpp"
#include "../../Graphics/Resources/Components/TextureInfoComponent.hpp"
#include "../../Graphics/GraphicsContexts.hpp"
#include "../../Graphics/Components/BufferManagerComponent.hpp"
#include "../../Graphics/Components/DescriptorManagerComponent.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "../../Graphics/Components/GlobalTransformComponent.hpp"
#include "../../Graphics/Components/LocalTransformComponent.hpp"
#include "../../Graphics/Components/RelationshipComponent.hpp"
#include "../../Graphics/Systems/TransformSystem.hpp"
#include "RotationSystem.hpp"
#include "../../Graphics/Systems/RenderSystem.hpp"
void SpawnSystem::update(float deltaTime, GeneralManager& gm) 
{
	time += deltaTime;
	if (time > 0.1)
	{
		time = 0;
		BufferManager* bufferManager =
		    gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
		BindlessTextureDSetComponent* dSetComponent =
		    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
		DescriptorManager* dManager =
		    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
		Entity gameObjectEntity1 = gm.createEntity();
		MeshInfoComponent meshInfo;
		int numberTexture;
		if (j % 2 == 0)
		{
			gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
			gm.addComponent<LocalTransformComponent>(gameObjectEntity1, k * -2.0f, 0.0f, j * -2.0f, 0.0f, 0.0f, 0.0f);
			meshInfo.mesh = bufferManager->createMesh("assets/models/Suzanne.glb");
			numberTexture = bufferManager->generateTextureData("assets/textures/texture.jpg", vk::Format::eR8G8B8A8Srgb,
			                                                   vk::ImageAspectFlagBits::eColor, *dSetComponent, *dManager);
		}
		else
		{
			gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
			gm.addComponent<LocalTransformComponent>(gameObjectEntity1, k * -2.0f, 0.0f, j * -2.0f, 0.0f, 0.0f, 0.0f,
			                                         5.0f, 5.0f, 5.0f );

			meshInfo.mesh = bufferManager->createMesh("assets/models/marble_bust_01_4k.glb");
			numberTexture =
			    bufferManager->generateTextureData("assets/textures/marble_bust_01_diff_4k.jpg", vk::Format::eR8G8B8A8Srgb,
			                                       vk::ImageAspectFlagBits::eColor, *dSetComponent, *dManager);
		}

		gm.addComponent<RelationshipComponent>(gameObjectEntity1);
		gm.addComponent<MeshInfoComponent>(gameObjectEntity1, meshInfo);
		gm.addComponent<TextureInfoComponent>(gameObjectEntity1, numberTexture);

		gm.subscribeEntity<TransformSystem>(gameObjectEntity1);
		gm.subscribeEntity<RotationSystem>(gameObjectEntity1);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);
		gm.subscribeEntity<SpawnSystem>(gameObjectEntity1);
		k++;
		if ((k + 1) % 10 == 0)
		{
			gm.destroyEntity(entities.front());
			j++;
			k = 0;
		}
	}
}

void SpawnSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "SpawnSystem registered!" << std::endl;
}

void SpawnSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "SpawnSystem shutdown!" << std::endl;
}


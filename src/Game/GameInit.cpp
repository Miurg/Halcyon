#include "GameInit.hpp"
#include "../Graphics/Resources/Components/MeshInfoComponent.hpp"
#include "../Graphics/Resources/Managers/BufferManager.hpp"
#include "../Graphics/Resources/Components/TextureInfoComponent.hpp"
#include "../Graphics/GraphicsContexts.hpp"
#include "../Graphics/Components/BufferManagerComponent.hpp"
#include "../Graphics/Components/DescriptorManagerComponent.hpp"
#include <vulkan/vulkan_raii.hpp>
#include "../Graphics/Components/GlobalTransformComponent.hpp"
#include "../Graphics/Components/LocalTransformComponent.hpp"
#include "../Graphics/Components/RelationshipComponent.hpp"
#include "../Graphics/Systems/TransformSystem.hpp"
#include "Systems/RotationSystem.hpp"
#include "../Graphics/Systems/RenderSystem.hpp"

void GameInit::gameInitStart(GeneralManager& gm)
{
	BufferManager* bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	BindlessTextureDSetComponent* dSetComponent = gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	int j = 0;
	int k = 0;
	for (int i = 0; i < 100; i++)
	{
		Entity gameObjectEntity1 = gm.createEntity();
		MeshInfoComponent meshInfo;
		int numberTexture;
		if (j%2 == 0)
		{
			gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
			gm.addComponent<LocalTransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f, 0.0f, 0.0f, 0.0f);
			meshInfo.mesh = bufferManager->createMesh("assets/models/Suzanne.glb", *dSetComponent, *dManager);
		}
		else
		{
			gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
			gm.addComponent<LocalTransformComponent>(gameObjectEntity1, k * 2.0f, 0.0f, j * 2.0f, -90.0f, 0.0f, 0.0f);
			

			meshInfo.mesh = bufferManager->createMesh("assets/models/marble_bust_01_4k.glb", *dSetComponent, *dManager);
		}

		gm.addComponent<RelationshipComponent>(gameObjectEntity1);
		gm.addComponent<MeshInfoComponent>(gameObjectEntity1, meshInfo);
		
		gm.subscribeEntity<TransformSystem>(gameObjectEntity1);
		gm.subscribeEntity<RotationSystem>(gameObjectEntity1);
		gm.subscribeEntity<RenderSystem>(gameObjectEntity1);

		k++;
		if ((i + 1) % 10 == 0)
		{
			RelationshipComponent* real = gm.getComponent<RelationshipComponent>(gameObjectEntity1);
			real->parent = gameObjectEntity1 - 1;
			RelationshipComponent* real2 = gm.getComponent<RelationshipComponent>(gameObjectEntity1 - 1);
			real2->firstChild = gameObjectEntity1;
			j++;
			k = 0;
		}

	}
}
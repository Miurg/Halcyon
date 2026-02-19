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
#include "../Graphics/Resources/Factories/ModelFactory.hpp"
#include "../Graphics/Components/TextureManagerComponent.hpp"
#include "../Graphics/Components/ModelManagerComponent.hpp"
#include "../Graphics/Components/CameraComponent.hpp"
#include "Components/ControlComponent.hpp"
#include "../Platform/PlatformContexts.hpp"

void GameInit::gameInitStart(GeneralManager& gm)
{
	BufferManager* bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* textureManager =
	    gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager* modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	BindlessTextureDSetComponent* dSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	DescriptorManager* dManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;

	gm.addComponent<ControlComponent>(gm.getContext<MainCameraContext>());

	Entity gameObjectEntity1 = gm.createEntity();
	gm.addComponent<GlobalTransformComponent>(gameObjectEntity1);
	gm.addComponent<LocalTransformComponent>(gameObjectEntity1);
	gm.addComponent<RelationshipComponent>(gameObjectEntity1);

	Entity dautherEntity = ModelFactory::loadModel("assets/models/Bistro_Godot.glb", 0, *bufferManager, *dSetComponent,
	                                               *dManager, gm, *textureManager, *modelManager);

	gm.subscribeEntity<TransformSystem>(gameObjectEntity1);

	RelationshipComponent* real = gm.getComponent<RelationshipComponent>(gameObjectEntity1);
	real->addChild(gameObjectEntity1, dautherEntity, gm);

	
	Entity gameObjectEntity2 = gm.createEntity();
	gm.addComponent<GlobalTransformComponent>(gameObjectEntity2);
	gm.addComponent<LocalTransformComponent>(gameObjectEntity2, -5.0f, 5.0f, 5.0f);
	gm.addComponent<RelationshipComponent>(gameObjectEntity2);

	Entity dautherEntity2 = ModelFactory::loadModel("assets/models/DamagedHelmet.glb", 0, *bufferManager, *dSetComponent,
	                                               *dManager, gm, *textureManager, *modelManager);

	gm.subscribeEntity<TransformSystem>(gameObjectEntity2);
	gm.subscribeEntity<RotationSystem>(gameObjectEntity2);
	RelationshipComponent* real2 = gm.getComponent<RelationshipComponent>(gameObjectEntity2);
	real2->addChild(gameObjectEntity2, dautherEntity2, gm);
}
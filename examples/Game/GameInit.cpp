#include "GameInit.hpp"

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>

#include <Game/Systems/ControlSystem.hpp>
#include <Game/Components/ControlComponent.hpp>

#include <GraphicsCore/Systems/DeltaTimeSystem.hpp>
#include <GraphicsCore/Systems/FrameBeginSystem.hpp>
#include <GraphicsCore/GraphicsContexts.hpp>
#include <GraphicsCore/Components/BufferManagerComponent.hpp>
#include <GraphicsCore/Components/TextureManagerComponent.hpp>
#include <GraphicsCore/Components/ModelManagerComponent.hpp>
#include <GraphicsCore/Components/DescriptorManagerComponent.hpp>
#include <GraphicsCore/Components/NameComponent.hpp>
#include <GraphicsCore/Components/RelationshipComponent.hpp>
#include <GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp>
#include <GraphicsCore/Components/VulkanDeviceComponent.hpp>
#include <GraphicsCore/Components/MaterialManagerComponent.hpp>
#include <GraphicsCore/Components/VMAllocatorComponent.hpp>
#include <GraphicsCore/Resources/Factories/ModelFactory.hpp>
#include <SmithCore/Renderables.hpp>

void GameInit::Run(GeneralManager& gm)
{
	// Fly-camera controls, attached to the engine's built-in main camera.
	gm.registerSystem<ControlSystem>()
	    .after<DeltaTimeSystem>()
	    .before<FrameBeginSystem>()
	    .reads<DeltaTimeComponent, KeyboardStateComponent, CameraComponent, ControlComponent>()
	    .writes<GlobalTransformComponent, CursorPositionComponent>();
	gm.addComponent<ControlComponent>(gm.getContext<MainCameraContext>());

	// Managers the model loader needs.
	BufferManager* bufferManager = gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	TextureManager* textureManager =
	    gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	ModelManager* modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	DescriptorManager* descriptorManager =
	    gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>()->descriptorManager;
	BindlessTextureDSetComponent* dSetComponent =
	    gm.getContextComponent<MainDSetsContext, BindlessTextureDSetComponent>();
	VulkanDevice& vulkanDevice =
	    *gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
	VmaAllocator allocator = gm.getContextComponent<VMAllocatorContext, VMAllocatorComponent>()->allocator;
	MaterialManager* materialManager =
	    gm.getContextComponent<MaterialManagerContext, MaterialManagerComponent>()->materialManager;

	Orhescyon::Entity cube = gm.createEntity();
	gm.addComponent<NameComponent>(cube, "Cube");
	Smith::Renderables::forgeTransform(gm, cube, glm::vec3(0.0f, 0.0f, -5.0f), glm::quat{1.0f, 0.0f, 0.0f, 0.0f});

	Orhescyon::Entity mesh = ModelFactory::loadModel("assets/models/cube.gltf", 0, *bufferManager, *dSetComponent,
	                                                 *descriptorManager, gm, *textureManager, *modelManager, *materialManager, vulkanDevice, allocator);
	gm.getComponent<RelationshipComponent>(cube)->addChild(cube, mesh, gm);
}

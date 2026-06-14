#pragma once

// Core graphics init and contexts
#include "../src/GraphicsCore/GraphicsInit/GraphicsInit.hpp"
#include "GraphicsCore/GraphicsContexts.hpp"

// Core graphics structures
#include "../src/GraphicsCore/VulkanDevice.hpp"
#include "../src/GraphicsCore/SwapChain.hpp"
#include "../src/GraphicsCore/FrameData.hpp"
#include "../src/GraphicsCore/VulkanConst.hpp"
#include "../src/GraphicsCore/VulkanUtils.hpp"
#include "../src/GraphicsCore/Resources/ResourceStructures.hpp"

// Components
#include "GraphicsCore/Components/AutoExposureSettingsComponent.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/CameraComponent.hpp"
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Components/ExposureBufferComponent.hpp"
#include "GraphicsCore/Components/FrameDataComponent.hpp"
#include "GraphicsCore/Components/FrameImageComponent.hpp"
#include "GraphicsCore/Components/FrameManagerComponent.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include "GraphicsCore/Components/ParticleEmitorComponent.hpp"
#include "GraphicsCore/Components/ParticlesBufferComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Components/RenderGraphComponent.hpp"
#include "GraphicsCore/Components/ShaderReloaderComponent.hpp"
#include "GraphicsCore/Components/SkyboxComponent.hpp"
#include "GraphicsCore/Components/SwapChainComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/VMAllocatorComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"

// Resource components
#include "GraphicsCore/Resources/Components/BindlessTextureDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/FrustumDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/MeshInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/TextureInfoComponent.hpp"

// Managers
#include "../src/GraphicsCore/Managers/FrameManager.hpp"
#include "../src/GraphicsCore/Managers/PipelineManager.hpp"
#include "../src/GraphicsCore/ShaderReloader.hpp"

// Resource managers
#include "../src/GraphicsCore/Resources/Managers/Bindings.hpp"
#include "../src/GraphicsCore/Resources/Managers/Buffer.hpp"
#include "../src/GraphicsCore/Resources/Managers/BufferManager.hpp"
#include "../src/GraphicsCore/Resources/Managers/DescriptorLayoutRegistry.hpp"
#include "../src/GraphicsCore/Resources/Managers/DescriptorManager.hpp"
#include "../src/GraphicsCore/Resources/Managers/MeshInfo.hpp"
#include "../src/GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "../src/GraphicsCore/Resources/Managers/PrimitivesInfo.hpp"
#include "../src/GraphicsCore/Resources/Managers/ResourceHandles.hpp"
#include "../src/GraphicsCore/Resources/Managers/Texture.hpp"
#include "../src/GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "../src/GraphicsCore/Resources/Managers/Vertex.hpp"
#include "../src/GraphicsCore/Resources/Managers/VertexIndexBuffer.hpp"

// Factories
#include "../src/GraphicsCore/Factories/PipelineBuilder.hpp"
#include "../src/GraphicsCore/Factories/PipelineFactory.hpp"
#include "../src/GraphicsCore/Factories/SwapChainFactory.hpp"
#include "../src/GraphicsCore/Factories/VulkanDeviceFactory.hpp"

// Resource factories
#include "../src/GraphicsCore/Resources/Factories/GltfLoader.hpp"
#include "../src/GraphicsCore/Resources/Factories/ImageConverter.hpp"
#include "../src/GraphicsCore/Resources/Factories/ModelFactory.hpp"
#include "../src/GraphicsCore/Resources/Factories/SkyboxFactory.hpp"
#include "../src/GraphicsCore/Resources/Factories/TextureUploader.hpp"

// Systems
#include "../src/GraphicsCore/Systems/BufferUpdateSystem.hpp"
#include "../src/GraphicsCore/Systems/CameraMatrixSystem.hpp"
#include "../src/GraphicsCore/Systems/DeltaTimeSystem.hpp"
#include "../src/GraphicsCore/Systems/FrameBeginSystem.hpp"
#include "../src/GraphicsCore/Systems/FrameEndSystem.hpp"
#include "../src/GraphicsCore/Systems/GPUParticlesSystem.hpp"
#include "../src/GraphicsCore/Systems/ImGuiSystem.hpp"
#include "../src/GraphicsCore/Systems/LightUpdateSystem.hpp"
#include "../src/GraphicsCore/Systems/PhysSyncSystem.hpp"
#include "../src/GraphicsCore/Systems/RenderSystem.hpp"
#include "../src/GraphicsCore/Systems/TransformSystem.hpp"

// Render graph
#include "../src/GraphicsCore/RenderGraph/RGPass.hpp"
#include "../src/GraphicsCore/RenderGraph/RGResource.hpp"
#include "../src/GraphicsCore/RenderGraph/RenderGraph.hpp"

// Passes
#include "../src/GraphicsCore/Passes/IPass.hpp"
#include "../src/GraphicsCore/Passes/BloomPass.hpp"
#include "../src/GraphicsCore/Passes/CullPass.hpp"
#include "../src/GraphicsCore/Passes/DebugPass.hpp"
#include "../src/GraphicsCore/Passes/DepthPrepass.hpp"
#include "../src/GraphicsCore/Passes/DepthPyramidPass.hpp"
#include "../src/GraphicsCore/Passes/DirectLightPass.hpp"
#include "../src/GraphicsCore/Passes/ExposurePass.hpp"
#include "../src/GraphicsCore/Passes/FXAAPass.hpp"
#include "../src/GraphicsCore/Passes/GTAOPass.hpp"
#include "../src/GraphicsCore/Passes/ImGuiPass.hpp"
#include "../src/GraphicsCore/Passes/MainPass.hpp"
#include "../src/GraphicsCore/Passes/ParticleSystemComputePass.hpp"
#include "../src/GraphicsCore/Passes/ParticleSystemRenderPass.hpp"
#include "../src/GraphicsCore/Passes/PassCommands.hpp"
#include "../src/GraphicsCore/Passes/PresentPass.hpp"
#include "../src/GraphicsCore/Passes/ToneMappingPass.hpp"
#include "../src/GraphicsCore/Passes/VignettePass.hpp"

// GI
#include "../src/GraphicsCore/GIBaker/LightProbeGIBaking.hpp"

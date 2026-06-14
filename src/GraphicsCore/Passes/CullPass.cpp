#include "CullPass.hpp"
#include "PassCommands.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/BufferManagerComponent.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/DescriptorManagerComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/DrawInfoComponent.hpp"
#include "GraphicsCore/Resources/Components/GlobalDSetComponent.hpp"
#include "GraphicsCore/Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../Factories/PipelineFactory.hpp"
#include "../RenderGraph/RenderGraph.hpp"

void CullPass::onInit(Orhescyon::GeneralManager& gm)
{
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	pManager.build(PipelineDescription{
	    .isCompute = true,
	    .shaderPath = "shaders/frustum_culling.spv",
	    .setLayoutNames = {"globalSet", "modelSet"},
	    .pushConstants = {{vk::ShaderStageFlagBits::eCompute, 0, sizeof(uint32_t)}},
	});
}

void CullPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	rg.addPass("ResetInstanceCount", {.isCompute = true}, {}, {}, [&, frame](vk::raii::CommandBuffer& cmd)
	           { drawResetInstancePass(cmd, frame, dManager, objectDSetComponent, drawInfo, pManager); });

	rg.addPass("Cull", {.isCompute = true}, {}, {},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawCullPass(cmd, frame, dManager, globalDSetComponent, objectDSetComponent, mManager, bManager,
		                        drawInfo, pManager);
	           });
}

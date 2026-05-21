#include "CullPass.hpp"
#include "PassCommands.hpp"

#include <Orhescyon/GeneralManager.hpp>

#include "../GraphicsContexts.hpp"
#include "../Components/BufferManagerComponent.hpp"
#include "../Components/ModelManagerComponent.hpp"
#include "../Components/DescriptorManagerComponent.hpp"
#include "../Components/PipelineManagerComponent.hpp"
#include "../Components/DrawInfoComponent.hpp"
#include "../Resources/Components/GlobalDSetComponent.hpp"
#include "../Resources/Components/ModelDSetComponent.hpp"
#include "../Resources/Managers/BufferManager.hpp"
#include "../Resources/Managers/ModelManager.hpp"
#include "../Managers/PipelineManager.hpp"
#include "../RenderGraph/RenderGraph.hpp"

void CullPass::addToGraph(Orhescyon::GeneralManager& gm, RenderGraph& rg, uint32_t frame)
{
	auto& dManager = *gm.getContextComponent<DescriptorManagerContext, DescriptorManagerComponent>();
	auto& globalDSetComponent = *gm.getContextComponent<MainDSetsContext, GlobalDSetComponent>();
	auto& bManager = *gm.getContextComponent<BufferManagerContext, BufferManagerComponent>()->bufferManager;
	auto& objectDSetComponent = *gm.getContextComponent<MainDSetsContext, ModelDSetComponent>();
	auto& mManager = *gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	auto& drawInfo = *gm.getContextComponent<CurrentFrameContext, DrawInfoComponent>();
	auto& pManager = *gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;

	rg.addPass("ResetInstanceCount", {.isCompute = true}, {}, {},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawResetInstancePass(cmd, frame, dManager, objectDSetComponent, drawInfo, pManager);
	           });

	rg.addPass("Cull", {.isCompute = true}, {}, {},
	           [&, frame](vk::raii::CommandBuffer& cmd)
	           {
		           drawCullPass(cmd, frame, dManager, globalDSetComponent, objectDSetComponent, mManager, bManager,
		                        drawInfo, pManager);
	           });
}

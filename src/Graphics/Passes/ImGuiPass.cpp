#include "ImGuiPass.hpp"

#include <imgui.h>
#include <imgui_impl_vulkan.h>

#include "../RenderGraph/RenderGraph.hpp"

void ImGuiPass::addToGraph(Orhescyon::GeneralManager& /*gm*/, RenderGraph& rg, uint32_t /*frame*/)
{
	rg.addPass("ImGui",
	           {.colorAttachments = {{"PostProcessColor", vk::AttachmentLoadOp::eLoad,
	                                  vk::AttachmentStoreOp::eStore}}},
	           {},
	           {{"PostProcessColor", RGResourceUsage::ColorAttachmentWrite}},
	           [](vk::raii::CommandBuffer& cmd)
	           {
		           ImDrawData* draw_data = ImGui::GetDrawData();
		           if (draw_data)
		           {
			           ImGui_ImplVulkan_RenderDrawData(draw_data, *cmd);
		           }
	           });
}

#include "GraphicsCore/Systems/DevSystems/ImGuiSystem.hpp"
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_vulkan.h>
#include <iostream>
#include <numeric>
#include <algorithm>
#include "GraphicsCore/GraphicsContexts.hpp"
#include "GraphicsCore/Components/GlobalTransformComponent.hpp"
#include "GraphicsCore/Components/CameraComponent.hpp"
#include "GraphicsCore/Components/DirectLightComponent.hpp"
#include "GraphicsCore/Components/LocalTransformComponent.hpp"
#include "GraphicsCore/Components/RelationshipComponent.hpp"
#include "GraphicsCore/Components/NameComponent.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <functional>
#include "GraphicsCore/Components/CurrentFrameComponent.hpp"
#include "GraphicsCore/Components/DeltaTimeComponent.hpp"
#include "GraphicsCore/Components/GtaoSettingsComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/AutoExposureSettingsComponent.hpp"
#include "GraphicsCore/Components/VulkanDeviceComponent.hpp"
#include "GraphicsCore/VulkanDevice.hpp"
#include "GraphicsCore/Components/PointLightComponent.hpp"
#include "GraphicsCore/ShaderReloader.hpp"
#include "GraphicsCore/Components/ShaderReloaderComponent.hpp"
#include "GraphicsCore/Components/PipelineManagerComponent.hpp"
#include "GraphicsCore/Components/GraphicsSettingsComponent.hpp"
#include "GraphicsCore/Components/LightProbeGridComponent.hpp"
#include "GraphicsCore/Components/ReflectionProbeComponent.hpp"
#include <Jolt/Jolt.h>
#include <Jolt/Physics/PhysicsSystem.h>
#include "PhysicsCore/PhysContexts.hpp"
#include "PhysicsCore/Components/PhysManagerComponent.hpp"
#include "PhysicsCore/Components/PhysBodyComponent.hpp"
#include "PhysicsCore/JoltGlm.hpp"
#include "GraphicsCore/Components/ModelManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/ModelManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Components/ModelComponent.hpp"
#include "GraphicsCore/Resources/Factories/ModelFactory.hpp"
#include "GraphicsCore/Systems/DevSystems/ComponentInspector.hpp"

#ifdef TRACY_ENABLE
#include <tracy/Tracy.hpp>
#endif

namespace
{
void drawArenaBar(const RangeAllocator& arena, const char* label, size_t elementSize)
{
	if (arena.capacity() == 0)
	{
		ImGui::Text("%s: empty arena", label);
		return;
	}

	const float barHeight = 18.0f;
	float barWidth = ImGui::GetContentRegionAvail().x;
	ImVec2 origin = ImGui::GetCursorScreenPos();
	ImDrawList* drawList = ImGui::GetWindowDrawList();

	drawList->AddRectFilled(origin, ImVec2(origin.x + barWidth, origin.y + barHeight), IM_COL32(190, 90, 70, 255));
	for (const RangeAllocator::Range& range : arena.freeRanges())
	{
		float x0 = origin.x + barWidth * (static_cast<float>(range.offset) / arena.capacity());
		float x1 = origin.x + barWidth * (static_cast<float>(range.offset + range.size) / arena.capacity());
		drawList->AddRectFilled(ImVec2(x0, origin.y), ImVec2(x1, origin.y + barHeight), IM_COL32(90, 190, 110, 255));
	}
	drawList->AddRect(origin, ImVec2(origin.x + barWidth, origin.y + barHeight), IM_COL32(255, 255, 255, 70));
	ImGui::Dummy(ImVec2(barWidth, barHeight));

	uint32_t usedElements = arena.capacity() - arena.totalFree();
	double usedMb = usedElements * static_cast<double>(elementSize) / (1024.0 * 1024.0);
	double capacityMb = arena.capacity() * static_cast<double>(elementSize) / (1024.0 * 1024.0);
	ImGui::Text("%s: %.2f / %.2f MB used, %zu free block(s)", label, usedMb, capacityMb, arena.freeRanges().size());
}

void drawMemoryWindow(GeneralManager& gm)
{
	ModelManager* modelManager = gm.getContextComponent<ModelManagerContext, ModelManagerComponent>()->modelManager;
	TextureManager* textureManager =
	    gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;

	ImGui::Begin("Memory");

	ImGui::SeparatorText("Model instances");
	// Snapshot first: unloading destroys entities and would invalidate iteration of the active set.
	std::vector<Orhescyon::Entity> modelRoots;
	gm.forEachActiveEntity(
	    [&](Orhescyon::Entity entity)
	    {
		    if (gm.hasComponent<ModelComponent>(entity)) modelRoots.push_back(entity);
	    });
	for (Orhescyon::Entity root : modelRoots)
	{
		ImGui::PushID(static_cast<int>(root.slot));
		int modelIndex = gm.getComponent<ModelComponent>(root)->modelIndex;
		auto* nameComp = gm.getComponent<NameComponent>(root);
		ImGui::Text("%u  %s (model %d, refs %d)", root.slot, nameComp ? nameComp->name : "?",
		            modelIndex, modelManager->getModel(modelIndex).refCount);
		ImGui::SameLine();
		if (ImGui::Button("Unload"))
		{
			ModelFactory::unloadModel(root, gm, *modelManager, *textureManager);
		}
		ImGui::PopID();
	}

	ImGui::SeparatorText("Geometry arenas");
	VertexIndexBuffer& geometryBuffer = modelManager->getVertexIndexBuffer(0);
	drawArenaBar(geometryBuffer.vertexAllocator, "Vertices", sizeof(Vertex));
	drawArenaBar(geometryBuffer.indexAllocator, "Indices", sizeof(uint32_t));
	ImGui::Text("Pending geometry frees: %zu", modelManager->pendingGeometryFreeCount());
	if (ImGui::Button("Defragment"))
	{
		modelManager->defragment(geometryBuffer);
	}

	ImGui::SeparatorText("Slot pools");
	ImGui::Text("Textures : %zu total, %zu free, %zu pending free", textureManager->textures.size(),
	            textureManager->freeTextureSlotCount(), textureManager->pendingTextureFreeCount());
	ImGui::Text("Materials: %zu total, %zu free, %zu pending free", textureManager->materials.size(),
	            textureManager->freeMaterialSlotCount(), textureManager->pendingMaterialFreeCount());
	ImGui::Text("Meshes   : %zu total, %zu free", modelManager->meshes.size(), modelManager->freeMeshSlotCount());
	ImGui::Text("Models   : %zu total, %zu free", modelManager->models.size(), modelManager->freeModelSlotCount());

	ImGui::SeparatorText("Loaded models");
	for (size_t i = 0; i < modelManager->models.size(); ++i)
	{
		const Model& model = modelManager->getModel(static_cast<int>(i));
		if (model.refCount <= 0) continue;
		ImGui::Text("[%zu] refs %d | meshes %zu | vtx %u | idx %u | tex %zu | mat %zu", i, model.refCount,
		            model.meshes.size(), model.allocation.vertexCount, model.allocation.indexCount,
		            model.textures.size(), model.materials.size());
	}

	ImGui::End();
}
} // namespace

void ImGuiSystem::onRegistered(GeneralManager& gm)
{
	std::cout << "ImGuiSystem registered!" << std::endl;
}

void ImGuiSystem::onShutdown(GeneralManager& gm)
{
	std::cout << "ImGuiSystem shutdown!" << std::endl;
}

void ImGuiSystem::drawEntityNode(Orhescyon::Entity entity, GeneralManager& gm)
{
	std::string entityName = "Entity " + std::to_string(entity.slot);
	auto* nameComp = gm.getComponent<NameComponent>(entity);
	if (nameComp)
	{
		entityName = nameComp->name;
	}

	auto* relComp = gm.getComponent<RelationshipComponent>(entity);
	bool isLeaf = (relComp == nullptr || relComp->firstChild == NULL_ENTITY);

	ImGuiTreeNodeFlags flags =
	    ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick | ImGuiTreeNodeFlags_SpanAvailWidth;
	if (isLeaf)
	{
		flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
	}
	if (selectedEntity == entity)
	{
		flags |= ImGuiTreeNodeFlags_Selected;
	}

	bool nodeOpen = ImGui::TreeNodeEx((void*)(intptr_t)entity.slot, flags, "%s", entityName.c_str());
	if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
	{
		selectedEntity = entity;
		if (auto* gs = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
			gs->selectedEntity = entity;
	}

	if (nodeOpen)
	{
		if (!isLeaf)
		{
			Orhescyon::Entity child = relComp->firstChild;
			while (child != NULL_ENTITY)
			{
				if (gm.isActive(child))
				{
					drawEntityNode(child, gm);
				}
				auto* childRel = gm.getComponent<RelationshipComponent>(child);
				if (childRel)
				{
					child = childRel->nextSibling;
				}
				else
				{
					child = NULL_ENTITY;
				}
			}
			ImGui::TreePop();
		}
	}
}

void ImGuiSystem::update(GeneralManager& gm)
{

#ifdef TRACY_ENABLE
	ZoneScopedN("ImGuiSystem");
#endif

	CurrentFrameComponent* currentFrameComp = gm.getContextComponent<CurrentFrameContext, CurrentFrameComponent>();
	if (!currentFrameComp || !currentFrameComp->frameValid)
	{
		return;
	}
	ImGui_ImplVulkan_NewFrame();
	ImGui_ImplGlfw_NewFrame();
	ImGui::NewFrame();
	ImGui::Begin("Main Camera Info");

	GlobalTransformComponent* mainCameraTransform =
	    gm.getContextComponent<MainCameraContext, GlobalTransformComponent>();
	CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();

	if (mainCameraTransform && mainCamera)
	{
		ImGui::SeparatorText("Transform");
		glm::vec3 camPos   = mainCameraTransform->getGlobalPosition();
		glm::vec3 camEuler = glm::degrees(glm::eulerAngles(mainCameraTransform->getGlobalRotation()));
		bool camChanged = false;
		if (ImGui::InputFloat3("Position", glm::value_ptr(camPos)))   camChanged = true;
		if (ImGui::InputFloat3("Rotation", glm::value_ptr(camEuler))) camChanged = true;
		if (camChanged)
		{
			mainCameraTransform->setGlobalPosition(camPos);
			mainCameraTransform->setGlobalRotation(glm::quat(glm::radians(camEuler)));
		}

		ImGui::SeparatorText("Camera Parameters");
		ImGui::DragFloat("FOV", &mainCamera->fov, 0.1f, 1.0f, 179.0f);
		ImGui::DragFloat("Near Plane", &mainCamera->zNear, 0.01f, 0.001f, 10.0f);
		ImGui::DragFloat("Far Plane", &mainCamera->zFar, 1.0f, 10.0f, 10000.0f);
		ImGui::DragFloat("Ortho size", &mainCamera->orthoSize, 0.1f, 0.1f, 100.0f);
	}
	float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

	ImGui::Text("FPS: %d", fps);
	ImGui::Text("Avg Frame time: %.2f ms", avgFrameTime * 1000.0f);
	ImGui::Text("1%% Low: %.2f ms", onePercentLowFrameTime * 1000.0f);
	frameCount++;
	time += deltaTime;
	frameTimes.push_back(deltaTime);

	if (time >= 1.0f)
	{
		fps = frameCount;

		if (!frameTimes.empty())
		{
			float sum = 0.0f;
			for (float ft : frameTimes) sum += ft;
			avgFrameTime = sum / frameTimes.size();

			std::vector<float> sortedTimes = frameTimes;
			std::sort(sortedTimes.begin(), sortedTimes.end(), std::greater<float>());

			size_t onePercentCount = std::max<size_t>(1, frameTimes.size() / 100);
			float worstSum = 0.0f;
			for (size_t i = 0; i < onePercentCount; ++i)
			{
				worstSum += sortedTimes[i];
			}
			onePercentLowFrameTime = worstSum / onePercentCount;
		}

		frameTimes.clear();
		frameCount = 0;
		time = 0;
	}

	ImGui::Checkbox("Enable shader auto reload", &autoShaderReload);
	if (autoShaderReload)
	{
		ShaderReloader* sr = gm.getContextComponent<ShaderReloaderContext, ShaderReloaderComponent>()->shaderReloader;
		PipelineManager* pipelineManager =
		    gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
		VulkanDevice* vulkanDevice =
		    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
		sr->update(*pipelineManager, *vulkanDevice);
	}

	if (auto* gs = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
	{
		ImGui::Checkbox("Draw AABB boxes on top", &gs->aabbAlwaysOnTop);
	}

	ImGui::End();

	// Entity Inspector Window
	ImGui::Begin("Entity Inspector");

	// Left Pane: Entity List
	ImGui::BeginChild("EntityList", ImVec2(350, 0), true);
	ImGui::Text("Active Entities");
	ImGui::Separator();

	gm.forEachActiveEntity(
	    [&](Orhescyon::Entity entity)
	    {
		    auto* relComp = gm.getComponent<RelationshipComponent>(entity);
		    // Top-level entities are those with no parent or no relationship component
		    if (relComp == nullptr || relComp->parent == NULL_ENTITY)
		    {
			    drawEntityNode(entity, gm);
		    }
	    });
	ImGui::EndChild();

	ImGui::SameLine();

	// Right Pane: Component Details
	ImGui::BeginChild("ComponentDetails", ImVec2(0, 0), true);
	if (selectedEntity != Orhescyon::Entity::invalid() && gm.isActive(selectedEntity))
	{
		ImGui::Text("Inspecting Entity %u", selectedEntity.slot);
		auto* nameComp = gm.getComponent<NameComponent>(selectedEntity);
		if (nameComp)
		{
			ImGui::SameLine();
			ImGui::InputText("##NameEdit", nameComp->name, sizeof(nameComp->name));
		}

		ImGui::Separator();

		static InspectorRegistry inspectors = buildInspectorRegistry();
		inspectors.inspectAll(gm, selectedEntity);

	}
	else
	{
		ImGui::Text("Select an entity to inspect.");
	}
	ImGui::EndChild();

	ImGui::End();

	drawMemoryWindow(gm);

	// ImGui::ShowDemoWindow();
}

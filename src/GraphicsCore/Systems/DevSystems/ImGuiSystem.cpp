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
#include "GraphicsCore/Components/RenderAssetManagerComponent.hpp"
#include "GraphicsCore/Components/TextureManagerComponent.hpp"
#include "GraphicsCore/Components/MaterialManagerComponent.hpp"
#include "GraphicsCore/Resources/Managers/MaterialManager.hpp"
#include "GraphicsCore/Resources/Managers/RenderAssetManager.hpp"
#include "GraphicsCore/Resources/Managers/TextureManager.hpp"
#include "GraphicsCore/Resources/Components/RenderAssetComponent.hpp"
#include "SmithCore/Renderables.hpp"
#include "GraphicsCore/Systems/DevSystems/ComponentInspector.hpp"
#include "GraphicsCore/Systems/DevSystems/FileInspector.hpp"
#include <exception>
#include <chrono>
#include <unordered_set>

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
}

struct ImGuiSystem::SceneCache
{
	struct TreeRow
	{
		Orhescyon::Entity entity;
		uint32_t depth;
		bool hasChildren = false;
	};

	struct HierarchyEntry
	{
		Orhescyon::Entity entity;
		Orhescyon::Entity parent;
		Orhescyon::Entity firstChild;
		Orhescyon::Entity nextSibling;
		bool operator==(const HierarchyEntry&) const = default;
	};

	std::vector<Orhescyon::Entity> roots;
	std::vector<Orhescyon::Entity> sceneInstances;
	std::vector<HierarchyEntry> hierarchy;
	std::vector<TreeRow> rows;
	std::vector<TreeRow> stack;
	std::vector<uint64_t> visitedSlots;
	std::unordered_set<Orhescyon::Entity> expandedEntities;
	std::chrono::steady_clock::time_point nextCheck{};
	uint32_t activeEntityCount = 0;
	bool dirty = true;
	bool treeDirty = true;

	void refresh(GeneralManager& gm)
	{
		const auto now = std::chrono::steady_clock::now();
		const uint32_t currentEntityCount = gm.activeEntityCount();
		// Periodic polling catches direct ECS edits and slot reuse even when the entity count stays unchanged.
		if (!dirty && currentEntityCount == activeEntityCount && now < nextCheck) return;

		roots.clear();
		sceneInstances.clear();
		size_t hierarchyIndex = 0;
		bool hierarchyChanged = false;
		{
			gm.forEachActiveEntity(
			    [&](Orhescyon::Entity entity)
			    {
				    auto* relationship = gm.getComponent<RelationshipComponent>(entity);
				    HierarchyEntry entry{entity, relationship ? relationship->parent : NULL_ENTITY,
				                         relationship ? relationship->firstChild : NULL_ENTITY,
				                         relationship ? relationship->nextSibling : NULL_ENTITY};
				    if (hierarchyIndex == hierarchy.size())
				    {
					    hierarchy.push_back(entry);
					    hierarchyChanged = true;
				    }
				    else if (!(hierarchy[hierarchyIndex] == entry))
				    {
					    hierarchy[hierarchyIndex] = entry;
					    hierarchyChanged = true;
				    }
				    ++hierarchyIndex;
				    if (!relationship || relationship->parent == NULL_ENTITY) roots.push_back(entity);
				    if (gm.hasComponent<RenderAssetComponent>(entity)) sceneInstances.push_back(entity);
			    });
		}
		if (hierarchyIndex != hierarchy.size())
		{
			hierarchy.resize(hierarchyIndex);
			hierarchyChanged = true;
		}
		if (hierarchyChanged)
			std::erase_if(expandedEntities, [&](Orhescyon::Entity entity) { return !gm.isActive(entity); });
		activeEntityCount = currentEntityCount;
		nextCheck = now + std::chrono::milliseconds(500);
		dirty = false;
		treeDirty = treeDirty || hierarchyChanged;
	}

	void rebuildTree(GeneralManager& gm)
	{
		rows.clear();
		stack.clear();
		std::fill(visitedSlots.begin(), visitedSlots.end(), uint64_t{0});
		for (auto it = roots.rbegin(); it != roots.rend(); ++it) stack.push_back({*it, 0});
		while (!stack.empty())
		{
			TreeRow row = stack.back();
			stack.pop_back();
			if (!gm.isActive(row.entity))
			{
				dirty = true;
				continue;
			}
			const size_t wordIndex = row.entity.slot / 64;
			const uint64_t mask = uint64_t{1} << (row.entity.slot % 64);
			if (wordIndex >= visitedSlots.size()) visitedSlots.resize(wordIndex + 1);
			if (visitedSlots[wordIndex] & mask) continue;
			visitedSlots[wordIndex] |= mask;

			auto* relationship = gm.getComponent<RelationshipComponent>(row.entity);
			if (row.depth > 0 && relationship && relationship->nextSibling != NULL_ENTITY)
				stack.push_back({relationship->nextSibling, row.depth});
			row.hasChildren = relationship && relationship->firstChild != NULL_ENTITY;
			rows.push_back(row);
			if (row.hasChildren && expandedEntities.contains(row.entity))
				stack.push_back({relationship->firstChild, row.depth + 1});
		}
		treeDirty = false;
	}
};

void ImGuiSystem::drawMemoryWindow(GeneralManager& gm)
{
	if (!ImGui::Begin("Memory"))
	{
		ImGui::End();
		return;
	}
	sceneCache->refresh(gm);

	RenderAssetManager* renderAssetManager =
	    gm.getContextComponent<RenderAssetManagerContext, RenderAssetManagerComponent>()->renderAssetManager;
	TextureManager* textureManager =
	    gm.getContextComponent<TextureManagerContext, TextureManagerComponent>()->textureManager;
	MaterialManager* materialManager =
	    gm.getContextComponent<MaterialManagerContext, MaterialManagerComponent>()->materialManager;

	ImGui::SeparatorText("Scene instances");
	{
		for (Orhescyon::Entity sceneInstance : sceneCache->sceneInstances)
		{
			if (!gm.isActive(sceneInstance) || !gm.hasComponent<RenderAssetComponent>(sceneInstance))
			{
				sceneCache->dirty = true;
				continue;
			}
			ImGui::PushID(static_cast<int>(sceneInstance.slot));
			RenderAssetHandle renderAssetHandle = gm.getComponent<RenderAssetComponent>(sceneInstance)->renderAsset;
			auto* nameComp = gm.getComponent<NameComponent>(sceneInstance);
			ImGui::Text("%u  %s (render asset %d, refs %d)", sceneInstance.slot, nameComp ? nameComp->name : "?",
			            renderAssetHandle.id, renderAssetManager->getRenderAsset(renderAssetHandle).refCount);
			ImGui::SameLine();
			if (ImGui::Button("Unload"))
			{
				sceneCache->dirty = true;
				Smith::Renderables::destroySceneInstance(gm, sceneInstance);
			}
			ImGui::PopID();
		}
	}

	{
		ImGui::SeparatorText("Geometry arenas");
		VertexIndexBuffer& geometryBuffer = renderAssetManager->getVertexIndexBuffer(0);
		drawArenaBar(geometryBuffer.vertexAllocator, "Vertices", sizeof(Vertex));
		drawArenaBar(geometryBuffer.indexAllocator, "Indices", sizeof(uint32_t));
		ImGui::Text("Pending geometry frees: %zu", renderAssetManager->pendingGeometryFreeCount());
		if (ImGui::Button("Defragment"))
		{
			renderAssetManager->defragment(geometryBuffer);
		}
	}

	{
		ImGui::SeparatorText("Slot pools");
		ImGui::Text("Textures : %zu total, %zu free, %zu pending free", textureManager->textureCount(),
		            textureManager->freeTextureSlotCount(), textureManager->pendingTextureFreeCount());
		ImGui::Text("Materials: %zu total, %zu free, %zu pending free", materialManager->materialCount(),
		            materialManager->freeMaterialSlotCount(), materialManager->pendingMaterialFreeCount());
		ImGui::Text("Meshes       : %zu total, %zu free", renderAssetManager->meshCount(),
		            renderAssetManager->freeMeshSlotCount());
		ImGui::Text("Render assets: %zu total, %zu free", renderAssetManager->renderAssetCount(),
		            renderAssetManager->freeRenderAssetSlotCount());
	}

	{
		ImGui::SeparatorText("Loaded render assets");
		for (size_t i = 0; i < renderAssetManager->renderAssetCount(); ++i)
		{
			const RenderAsset& renderAsset = renderAssetManager->getRenderAsset(RenderAssetHandle{static_cast<int>(i)});
			if (renderAsset.refCount <= 0) continue;
			ImGui::Text("[%zu] refs %d | meshes %zu | vtx %u | idx %u | tex %zu | mat %zu", i, renderAsset.refCount,
			            renderAsset.meshes.size(), renderAsset.allocation.vertexCount, renderAsset.allocation.indexCount,
			            renderAsset.textures.size(), renderAsset.materials.size());
		}
	}

	ImGui::End();
}

ImGuiSystem::ImGuiSystem() = default;
ImGuiSystem::~ImGuiSystem() = default;

void ImGuiSystem::onRegistered(GeneralManager& gm)
{
	sceneCache = std::make_unique<SceneCache>();
	fileInspector = std::make_unique<FileInspector>();
	std::cout << "ImGuiSystem registered!" << std::endl;
}

void ImGuiSystem::onShutdown(GeneralManager& gm)
{
	sceneCache.reset();
	fileInspector.reset();
	std::cout << "ImGuiSystem shutdown!" << std::endl;
}

void ImGuiSystem::drawEntityTree(GeneralManager& gm)
{
	sceneCache->refresh(gm);
	if (sceneCache->treeDirty) sceneCache->rebuildTree(gm);

	const float indentSpacing = ImGui::GetStyle().IndentSpacing;
	ImGuiListClipper clipper;
	clipper.Begin(static_cast<int>(sceneCache->rows.size()), ImGui::GetTextLineHeightWithSpacing());
	while (clipper.Step())
	{
		for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
		{
			const auto& row = sceneCache->rows[i];
			if (!gm.isActive(row.entity))
			{
				sceneCache->dirty = true;
				ImGui::Dummy(ImVec2(0.0f, ImGui::GetTextLineHeight()));
				continue;
			}

			ImGui::PushID(static_cast<int>(row.entity.slot));
			ImGui::PushID(static_cast<int>(row.entity.generation));
			const float indent = row.depth * indentSpacing;
			if (indent > 0.0f) ImGui::Indent(indent);

			ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
			                           ImGuiTreeNodeFlags_SpanAvailWidth | ImGuiTreeNodeFlags_NoTreePushOnOpen;
			if (!row.hasChildren) flags |= ImGuiTreeNodeFlags_Leaf;
			if (selectedEntity == row.entity) flags |= ImGuiTreeNodeFlags_Selected;
			ImGui::SetNextItemOpen(sceneCache->expandedEntities.contains(row.entity), ImGuiCond_Always);
			auto* name = gm.getComponent<NameComponent>(row.entity);
			const bool nodeOpen = name ? ImGui::TreeNodeEx("##Entity", flags, "%s", name->name)
			                           : ImGui::TreeNodeEx("##Entity", flags, "Entity %u", row.entity.slot);
			if (row.hasChildren && ImGui::IsItemToggledOpen())
			{
				if (nodeOpen)
					sceneCache->expandedEntities.insert(row.entity);
				else
					sceneCache->expandedEntities.erase(row.entity);
				// Keep the clipper's row indices stable until the next frame.
				sceneCache->treeDirty = true;
			}
			if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen())
			{
				selectedEntity = row.entity;
				if (auto* settings = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
					settings->selectedEntity = row.entity;
			}

			if (indent > 0.0f) ImGui::Unindent(indent);
			ImGui::PopID();
			ImGui::PopID();
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
	if (selectedEntity != NULL_ENTITY && !gm.isActive(selectedEntity))
	{
		if (auto* settings = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
			if (settings->selectedEntity == selectedEntity) settings->selectedEntity = NULL_ENTITY;
		selectedEntity = NULL_ENTITY;
	}
	{
		ImGui_ImplVulkan_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();
	}
	{
		const bool cameraVisible = ImGui::Begin("Main Camera Info");

		if (cameraVisible)
		{
			GlobalTransformComponent* mainCameraTransform =
			    gm.getContextComponent<MainCameraContext, GlobalTransformComponent>();
			CameraComponent* mainCamera = gm.getContextComponent<MainCameraContext, CameraComponent>();

			if (mainCameraTransform && mainCamera)
			{
				ImGui::SeparatorText("Transform");
				glm::vec3 camPos = mainCameraTransform->getGlobalPosition();
				glm::vec3 camEuler = glm::degrees(glm::eulerAngles(mainCameraTransform->getGlobalRotation()));
				bool camChanged = false;
				if (ImGui::InputFloat3("Position", glm::value_ptr(camPos))) camChanged = true;
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
		}
		{
			float deltaTime = gm.getContextComponent<DeltaTimeContext, DeltaTimeComponent>()->deltaTime;

			if (cameraVisible)
			{
				ImGui::Text("FPS: %d", fps);
				ImGui::Text("Avg Frame time: %.2f ms", avgFrameTime * 1000.0f);
				ImGui::Text("1%% Low: %.2f ms", onePercentLowFrameTime * 1000.0f);
			}
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
		}

		if (cameraVisible) ImGui::Checkbox("Enable shader auto reload", &autoShaderReload);
		if (autoShaderReload)
		{
			ShaderReloader* sr = gm.getContextComponent<ShaderReloaderContext, ShaderReloaderComponent>()->shaderReloader;
			PipelineManager* pipelineManager =
			    gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
			VulkanDevice* vulkanDevice =
			    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
			sr->update(*pipelineManager, *vulkanDevice);
		}

		if (cameraVisible)
			if (auto* gs = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
			{
				ImGui::Checkbox("Draw AABB boxes on top", &gs->aabbAlwaysOnTop);
			}

		ImGui::End();
	}

	if (const auto path = fileInspector->draw())
	{
		sceneCache->dirty = true;
		try
		{
			const auto utf8Path = path->u8string();
			const std::string modelPath(utf8Path.begin(), utf8Path.end());
			selectedEntity = Smith::Renderables::forgeSceneInstance(gm, modelPath.c_str());
			if (auto* settings = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>())
				settings->selectedEntity = selectedEntity;
		}
		catch (const std::exception& error)
		{
			fileInspector->setError(error.what());
		}
	}

	{
		if (ImGui::Begin("Entity Inspector"))
		{
			if (ImGui::BeginChild("EntityList", ImVec2(350, 0), true))
			{
				ImGui::Text("Active Entities");
				ImGui::Separator();
				drawEntityTree(gm);
			}
			ImGui::EndChild();

			ImGui::SameLine();

			if (ImGui::BeginChild("ComponentDetails", ImVec2(0, 0), true))
			{
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
			}
			ImGui::EndChild();
		}

		ImGui::End();
	}

	drawMemoryWindow(gm);

	// ImGui::ShowDemoWindow();
}

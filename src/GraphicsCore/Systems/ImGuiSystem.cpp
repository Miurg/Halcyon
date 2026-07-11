#include "GraphicsCore/Systems/ImGuiSystem.hpp"
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
		            modelIndex, modelManager->models[modelIndex].refCount);
		ImGui::SameLine();
		if (ImGui::Button("Unload"))
		{
			ModelFactory::unloadModel(root, gm, *modelManager, *textureManager);
		}
		ImGui::PopID();
	}

	ImGui::SeparatorText("Geometry arenas");
	VertexIndexBuffer& geometryBuffer = modelManager->vertexIndexBuffers[0];
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
		const Model& model = modelManager->models[i];
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
		PipelineManager* pManager =
		    gm.getContextComponent<PipelineManagerContext, PipelineManagerComponent>()->pipelineManager;
		VulkanDevice* vulkanDevice =
		    gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>()->vulkanDeviceInstance;
		sr->update(*pManager, *vulkanDevice);
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

		// Global Transform Component
		if (auto* transform = gm.getComponent<GlobalTransformComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Global Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				glm::vec3 gPos   = transform->getGlobalPosition();
				glm::vec3 gEuler = glm::degrees(glm::eulerAngles(transform->getGlobalRotation()));
				glm::vec3 gScale = transform->getGlobalScale();
				bool gChanged = false;
				if (ImGui::DragFloat3("G Position", glm::value_ptr(gPos),   0.1f)) gChanged = true;
				if (ImGui::DragFloat3("G Rotation", glm::value_ptr(gEuler), 0.5f)) gChanged = true;
				if (ImGui::DragFloat3("G Scale",    glm::value_ptr(gScale), 0.1f)) gChanged = true;
				if (gChanged)
				{
					glm::quat newRot = glm::quat(glm::radians(gEuler));
					transform->setGlobalPosition(gPos);
					transform->setGlobalRotation(newRot);
					transform->setGlobalScale(gScale);

					if (auto* physBody = gm.getComponent<PhysBodyComponent>(selectedEntity))
					{
						auto* physMgrComp = gm.getContextComponent<PhysManagerContext, PhysManagerComponent>();
						JPH::BodyInterface& bi = physMgrComp->physManager->physicsSystem->GetBodyInterface();
						bi.SetPositionAndRotation(
						    physBody->bodyID,
						    JPH::RVec3(gPos.x, gPos.y, gPos.z),
						    toJolt(newRot),
						    JPH::EActivation::Activate);
					}
				}
			}
		}

		// Local Transform Component
		if (auto* localTransform = gm.getComponent<LocalTransformComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Local Transform", ImGuiTreeNodeFlags_DefaultOpen))
			{
				glm::vec3 lPos   = localTransform->getLocalPosition();
				glm::vec3 lEuler = glm::degrees(glm::eulerAngles(localTransform->getLocalRotation()));
				glm::vec3 lScale = localTransform->getLocalScale();
				bool lChanged = false;
				if (ImGui::DragFloat3("L Position", glm::value_ptr(lPos),   0.1f)) lChanged = true;
				if (ImGui::DragFloat3("L Rotation", glm::value_ptr(lEuler), 0.5f)) lChanged = true;
				if (ImGui::DragFloat3("L Scale",    glm::value_ptr(lScale), 0.1f)) lChanged = true;
				if (lChanged)
				{
					glm::quat newLocalRot = glm::quat(glm::radians(lEuler));
					localTransform->setLocalPosition(lPos);
					localTransform->setLocalRotation(newLocalRot);
					localTransform->setLocalScale(lScale);

					if (auto* physBody = gm.getComponent<PhysBodyComponent>(selectedEntity))
					{
						auto* physMgrComp = gm.getContextComponent<PhysManagerContext, PhysManagerComponent>();
						JPH::BodyInterface& bi = physMgrComp->physManager->physicsSystem->GetBodyInterface();

						glm::vec3 worldPos = lPos;
						glm::quat worldRot = newLocalRot;
						if (auto* rel = gm.getComponent<RelationshipComponent>(selectedEntity))
						{
							if (rel->parent != NULL_ENTITY)
							{
								auto* pg = gm.getComponent<GlobalTransformComponent>(rel->parent);
								worldPos = pg->getGlobalPosition() + pg->getGlobalRotation() * (pg->getGlobalScale() * lPos);
								worldRot = pg->getGlobalRotation() * newLocalRot;
							}
						}

						bi.SetPositionAndRotation(
						    physBody->bodyID,
						    JPH::RVec3(worldPos.x, worldPos.y, worldPos.z),
						    toJolt(worldRot),
						    JPH::EActivation::Activate);
					}
				}
			}
		}

		// Light Component
		if (auto* light = gm.getComponent<DirectLightComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Light Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SeparatorText("Sun Lighting");
				ImGui::ColorEdit3("Color", glm::value_ptr(light->color));
				ImGui::DragFloat("Intensity", &light->color.w, 0.01f, 0.0f, 1000.0f);

				ImGui::SeparatorText("Ambient Lighting");
				ImGui::ColorEdit3("Ambient Color", glm::value_ptr(light->ambient));
				ImGui::DragFloat("Ambient Intensity", &light->ambient.w, 0.001f, 0.0f, 10.0f);

				ImGui::SeparatorText("Shadow Settings");
				ImGui::DragFloat("Shadow Distance", &light->shadowDistance, 1.0f);
				ImGui::DragFloat("Shadow Caster Range", &light->shadowCasterRange, 10.0f);
			}
		}

		// Camera Component
		if (auto* camera = gm.getComponent<CameraComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Camera Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat("FOV", &camera->fov, 0.1f, 1.0f, 179.0f);
				ImGui::DragFloat("Near Plane", &camera->zNear, 0.01f, 0.001f, 10.0f);
				ImGui::DragFloat("Far Plane", &camera->zFar, 1.0f, 10.0f, 10000.0f);
				ImGui::DragFloat("Ortho size", &camera->orthoSize, 0.1f, 0.1f, 100.0f);
			}
		}

		// GTAO Settings Component
		if (auto* gtao = gm.getComponent<GtaoSettingsComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("GTAO Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::SliderInt("Kernel Size", &gtao->kernelSize, 1, 32);
				ImGui::DragFloat("Radius", &gtao->radius, 0.1f, 0.1f, 50.0f);
				ImGui::DragFloat("Bias", &gtao->bias, 0.01f, 0.0f, 1.0f);
				ImGui::DragFloat("Power", &gtao->power, 0.1f, 0.1f, 20.0f);
				ImGui::SliderInt("Num Directions", &gtao->numDirections, 1, 16);
				ImGui::DragFloat("Max Screen Radius", &gtao->maxScreenRadius, 0.01f, 0.01f, 1.0f);
				ImGui::DragFloat("Fade start", &gtao->fadeStart, 10.0f, 1000.0f);
				ImGui::DragFloat("Fade end", &gtao->fadeEnd, 10.0f, 1000.0f);
				ImGui::SliderFloat("Mip Bias", &gtao->mipBias, -4.0f, 4.0f);
				ImGui::SliderFloat("Blur Depth Tolerance", &gtao->blurDepthTolerance, 0.001f, 1.0f);
				ImGui::SliderFloat("Pyramid Edge Range", &gtao->pyramidEdgeRange, 0.0f, 2.0f);
				ImGui::SliderFloat("Multi-bounce Albedo", &gtao->multiBounceAlbedo, 0.0f, 1.0f);
				ImGui::SliderFloat("Thickness Scale", &gtao->thicknessScale, 0.1f, 5.0f);
			}
		}

		// Graphics Settings Component
		if (auto* settings = gm.getComponent<GraphicsSettingsComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Graphics Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::Checkbox("Enable GTAO", &settings->enableGtao);
				ImGui::Checkbox("Enable FXAA", &settings->enableFxaa);
				ImGui::Checkbox("Enable Bloom", &settings->enableBloom);
				ImGui::Checkbox("Enable Vignette", &settings->enableVignette);
				ImGui::Checkbox("Enable Auto Exposure", &settings->enableAutoExposure);
				if (settings->enableBloom)
				{
					ImGui::DragFloat("Bloom Threshold", &settings->bloomThreshold, 0.1f, 0.0f, 10.0f);
					ImGui::DragFloat("Bloom Knee", &settings->bloomKnee, 0.01f, 0.0f, 1.0f);
					ImGui::DragFloat("Bloom Intensity", &settings->bloomIntensity, 0.001f, 0.0f, 1.0f);
				}

				ImGui::SeparatorText("Color Grading");
				const char* gradingSpaces[] = { "Display (post-tonemap)", "Linear (HDR)", "Log" };
				ImGui::Combo("Grading Space", &settings->gradingSpace, gradingSpaces, IM_ARRAYSIZE(gradingSpaces));
				ImGui::SliderFloat("Exposure", &settings->colorExposure, -4.0f, 4.0f);
				ImGui::SliderFloat("Contrast", &settings->contrast, 0.0f, 2.0f);
				ImGui::SliderFloat("Saturation", &settings->saturation, 0.0f, 2.0f);
				ImGui::SliderFloat("Temperature", &settings->temperature, -1.0f, 1.0f);
				ImGui::SliderFloat("Tint", &settings->tint, -1.0f, 1.0f);
				if (ImGui::Button("Reset Color Grading"))
				{
					settings->gradingSpace = 2;
					settings->colorExposure = 0.0f;
					settings->contrast = 1.0f;
					settings->saturation = 1.0f;
					settings->temperature = 0.0f;
					settings->tint = 0.0f;
				}

				auto* vulkanDeviceComponent = gm.getContextComponent<MainVulkanDeviceContext, VulkanDeviceComponent>();
				if (vulkanDeviceComponent && vulkanDeviceComponent->vulkanDeviceInstance)
				{
					vk::SampleCountFlagBits maxSamples = vulkanDeviceComponent->vulkanDeviceInstance->maxMsaaSamples;
					int maxIndex = 0;
					if (maxSamples >= vk::SampleCountFlagBits::e64) maxIndex = 6;
					else if (maxSamples >= vk::SampleCountFlagBits::e32) maxIndex = 5;
					else if (maxSamples >= vk::SampleCountFlagBits::e16) maxIndex = 4;
					else if (maxSamples >= vk::SampleCountFlagBits::e8) maxIndex = 3;
					else if (maxSamples >= vk::SampleCountFlagBits::e4) maxIndex = 2;
					else if (maxSamples >= vk::SampleCountFlagBits::e2) maxIndex = 1;

					const char* allItems[] = { "Off (1x)", "2x", "4x", "8x", "16x", "32x", "64x" };
					
					int item_current = 0;
					switch(settings->msaaSamples) {
						case vk::SampleCountFlagBits::e1: item_current = 0; break;
						case vk::SampleCountFlagBits::e2: item_current = 1; break;
						case vk::SampleCountFlagBits::e4: item_current = 2; break;
						case vk::SampleCountFlagBits::e8: item_current = 3; break;
						case vk::SampleCountFlagBits::e16: item_current = 4; break;
						case vk::SampleCountFlagBits::e32: item_current = 5; break;
						case vk::SampleCountFlagBits::e64: item_current = 6; break;
						default: break;
					}
					
					// Clamp current item to max supported just in case
					if (item_current > maxIndex) 
					{
						item_current = maxIndex;
						// Force update the setting itself so the user doesn't crash on boot if they somehow saved a bad setting
						switch(maxIndex) {
							case 0: settings->msaaSamples = vk::SampleCountFlagBits::e1; break;
							case 1: settings->msaaSamples = vk::SampleCountFlagBits::e2; break;
							case 2: settings->msaaSamples = vk::SampleCountFlagBits::e4; break;
							case 3: settings->msaaSamples = vk::SampleCountFlagBits::e8; break;
							case 4: settings->msaaSamples = vk::SampleCountFlagBits::e16; break;
							case 5: settings->msaaSamples = vk::SampleCountFlagBits::e32; break;
							case 6: settings->msaaSamples = vk::SampleCountFlagBits::e64; break;
						}
					}

					if (ImGui::Combo("MSAA", &item_current, allItems, maxIndex + 1))
					{
						switch(item_current) {
							case 0: settings->msaaSamples = vk::SampleCountFlagBits::e1; break;
							case 1: settings->msaaSamples = vk::SampleCountFlagBits::e2; break;
							case 2: settings->msaaSamples = vk::SampleCountFlagBits::e4; break;
							case 3: settings->msaaSamples = vk::SampleCountFlagBits::e8; break;
							case 4: settings->msaaSamples = vk::SampleCountFlagBits::e16; break;
							case 5: settings->msaaSamples = vk::SampleCountFlagBits::e32; break;
							case 6: settings->msaaSamples = vk::SampleCountFlagBits::e64; break;
						}
					}
				}
			}
		}

		// Auto Exposure Settings Component
		if (auto* ae = gm.getComponent<AutoExposureSettingsComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Auto Exposure Settings", ImGuiTreeNodeFlags_DefaultOpen))
			{
				auto* gs = gm.getContextComponent<GraphicsSettingsContext, GraphicsSettingsComponent>();
				if (!gs || gs->enableAutoExposure)
				{
					ImGui::DragFloat("Brighten Speed", &ae->tauUp, 0.01f, 0.0f, 5.0f);
					ImGui::DragFloat("Darken Speed", &ae->tauDown, 0.01f, 0.0f, 5.0f);
					ImGui::DragFloat("Darkest Exposure", &ae->minEV, 0.1f, -20.0f, 0.0f);
					ImGui::DragFloat("Brightest Exposure", &ae->maxEV, 0.1f, 0.0f, 20.0f);
					ImGui::DragFloat("Target Brightness", &ae->targetLuminance, 0.01f, 0.0f, 2.0f);
					ImGui::SliderFloat("Shadow Cutoff", &ae->lowPercent, 0.0f, 1.0f);
					ImGui::SliderFloat("Highlight Cutoff", &ae->highPercent, 0.0f, 1.0f);
				}
				else
				{
					ImGui::TextDisabled("Auto Exposure is disabled in Graphics Settings.");
				}
			}
		}

		// Point Light Component
		if (auto* settings = gm.getComponent<PointLightComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Point Light Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::ColorEdit3("Color", glm::value_ptr(settings->color));
				ImGui::DragFloat("Intensity", &settings->intensity, 1.0f, 0.0f, 1000.0f);
				ImGui::DragFloat("Radius", &settings->radius, 0.1f, 0.1f, 100.0f);
				ImGui::DragFloat3("Direction", glm::value_ptr(settings->direction), 0.1f);
				ImGui::DragFloat("Inner Cone Angle", &settings->innerConeAngle, 0.1f, 0.0f, 180.0f);
				ImGui::DragFloat("Outer Cone Angle", &settings->outerConeAngle, 0.1f, 0.0f, 180.0f);
				const char* typeItems[] = { "Point Light", "Spot Light" };
				int typeCurrent = settings->type;
				if (ImGui::Combo("Type", &typeCurrent, typeItems, IM_ARRAYSIZE(typeItems)))
				{
					settings->type = typeCurrent;
				}
			}
		}

		if (auto* globalIllumination = gm.getComponent<LightProbeGridComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Global Illumination Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Origin", &globalIllumination->origin.x, 0.1f);

				int count[3] = { globalIllumination->count.x, globalIllumination->count.y, globalIllumination->count.z };
				if (ImGui::DragInt3("Count", count, 1, 1, 64))
				{
					globalIllumination->count.x = glm::max(count[0], 1);
					globalIllumination->count.y = glm::max(count[1], 1);
					globalIllumination->count.z = glm::max(count[2], 1);
				}

				ImGui::DragFloat("Spacing", &globalIllumination->spacing, 0.05f, 0.01f, 10.0f);
				ImGui::DragFloat("Capture Range", &globalIllumination->captureRange, 1.0f, 1.0f, 1000.0f);

				ImGui::SeparatorText("GI Ambient");
				ImGui::ColorEdit3("GI Ambient Color", glm::value_ptr(globalIllumination->giAmbientColor));
				ImGui::DragFloat("GI Ambient Intensity", &globalIllumination->giAmbientIntensity, 0.001f, 0.0f, 10.0f);
				ImGui::DragFloat("GI Bounce Multiplier", &globalIllumination->giBounceMultiplier, 0.01f, 0.0f, 10.0f);

				if (ImGui::Button("Bake Global Illumination", ImVec2(200, 20)))
				{
					globalIllumination->needBake = true;
				}

				ImGui::Checkbox("Visualize Probes", &globalIllumination->debugVisualize);
				if (globalIllumination->debugVisualize)
					ImGui::SliderFloat("Probe Scale", &globalIllumination->debugScale, 0.05f, 2.0f);
			}
		}

		if (auto* reflectionProbe = gm.getComponent<ReflectionProbeComponent>(selectedEntity))
		{
			if (ImGui::CollapsingHeader("Reflection Probe Component", ImGuiTreeNodeFlags_DefaultOpen))
			{
				ImGui::DragFloat3("Probe Origin", &reflectionProbe->origin.x, 0.1f);
				ImGui::DragFloat3("Influence Half-Extent", &reflectionProbe->halfExtent.x, 0.1f, 0.1f, 1000.0f);
				ImGui::DragFloat("Capture Range##refl", &reflectionProbe->captureRange, 1.0f, 1.0f, 1000.0f);

				ImGui::SeparatorText("Capture GI");
				ImGui::ColorEdit3("GI Ambient Color##refl", glm::value_ptr(reflectionProbe->giAmbientColor));
				ImGui::DragFloat("GI Ambient Intensity##refl", &reflectionProbe->giAmbientIntensity, 0.001f, 0.0f, 10.0f);
				ImGui::DragFloat("GI Bounce Multiplier##refl", &reflectionProbe->giBounceMultiplier, 0.01f, 0.0f, 10.0f);

				if (ImGui::Button("Bake Reflection Probe", ImVec2(200, 20)))
					reflectionProbe->needBake = true;
			}
		}

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

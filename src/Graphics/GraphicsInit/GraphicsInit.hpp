#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include "../Resources/Factories/GltfLoader.hpp"

using Orhescyon::GeneralManager;

class GraphicsInit
{
public:
	static void Run(GeneralManager& gm);
	static void recreateMsaaPipelines(GeneralManager& gm, vk::SampleCountFlagBits msaaSamples);

private:
	static void initVulkanCore(GeneralManager& gm);
	static void initManagers(GeneralManager& gm);
	static void initFrameData(GeneralManager& gm);
	static void initPipelines(GeneralManager& gm);
	static void initScene(GeneralManager& gm);
	static void initImGui(GeneralManager& gm);
};
#pragma once
#include "../Core/GeneralManager.hpp"
#include "Resources/Factories/GltfLoader.hpp"

class GraphicsInit
{
public:
	static void Run(GeneralManager& gm);

private:
	static void initVulkanCore(GeneralManager& gm);
	static void initManagers(GeneralManager& gm);
	static void initFrameData(GeneralManager& gm);
	static void initPipelines(GeneralManager& gm);
	static void initScene(GeneralManager& gm);
};
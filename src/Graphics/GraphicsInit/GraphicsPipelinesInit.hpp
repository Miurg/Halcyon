#pragma once
#include <Orhescyon/GeneralManager.hpp>
#include "../Resources/Factories/GltfLoader.hpp"

using Orhescyon::GeneralManager;

class GraphicsPipelinesInit
{
public:
	static void initPipelines(GeneralManager& gm);
	static void recreateMsaaPipelines(GeneralManager& gm, vk::SampleCountFlagBits msaaSamples);
};
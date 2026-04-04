#pragma once
#include <Orhescyon/GeneralManager.hpp>

using Orhescyon::GeneralManager;

class GraphicsPipelinesInit
{
public:
	static void initPipelines(GeneralManager& gm);
	static void recreateMsaaPipelines(GeneralManager& gm, vk::SampleCountFlagBits msaaSamples);
};
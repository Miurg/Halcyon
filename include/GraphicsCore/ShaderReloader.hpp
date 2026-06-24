#pragma once

#include "HalcyonExport.hpp"
#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/VulkanDevice.hpp"

class HALCYON_API ShaderReloader
{
public:
	ShaderReloader(const std::string& shadersDir = "shaders", const std::string& outDir = "shaders");
	void update(PipelineManager& pManager, VulkanDevice& device);

private:
	struct HALCYON_API ShaderInfo
	{
		std::filesystem::file_time_type lastWriteTime;
		std::string name; // filename without extension
	};

	std::string shadersDir;
	std::string outDir;
	std::unordered_map<std::string, ShaderInfo> trackedShaders;

	bool compileShader(const std::string& slangPath, const std::string& slangContent, const std::string& outSpvPath);
	void scanDirectory();
	std::string readFileContent(const std::string& filepath);
};

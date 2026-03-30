#pragma once

#include <string>
#include <vector>
#include <filesystem>
#include <unordered_map>
#include "Managers/PipelineManager.hpp"
#include "VulkanDevice.hpp"

class ShaderReloader
{
public:
	ShaderReloader(const std::string& shadersDir = "assets/shaders", const std::string& outDir = "shaders");
	void update(PipelineManager& pManager, VulkanDevice& device);

private:
	struct ShaderInfo
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

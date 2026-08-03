#pragma once

#include <chrono>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "GraphicsCore/Managers/PipelineManager.hpp"
#include "GraphicsCore/VulkanDevice.hpp"

class ShaderReloader
{
public:
	ShaderReloader(const std::string& shadersDir = "shaders", const std::string& outDir = "shaders");
	void update(PipelineManager& pipelineManager, VulkanDevice& device);

private:
	struct ShaderInfo
	{
		std::filesystem::path sourcePath;
		std::string outputName;
		std::vector<std::string> entryPoints;
	};

	std::string shadersDir;
	std::string outDir;
	std::unordered_map<std::string, std::filesystem::file_time_type> trackedFiles;
	std::unordered_map<std::string, ShaderInfo> entryShaders;
	std::unordered_map<std::string, std::unordered_set<std::string>> dependentShaders;
	std::chrono::steady_clock::time_point nextPollTime{};

	bool compileShader(const ShaderInfo& shader, const std::string& outSpvPath);
	void scanDirectory();
	void collectDependencies(const std::filesystem::path& sourcePath,
	                         std::unordered_set<std::string>& dependencies,
	                         std::unordered_set<std::string>& visited) const;
	std::string readFileContent(const std::string& filepath) const;
};

#include "ShaderReloader.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>

ShaderReloader::ShaderReloader(const std::string& shadersDir, const std::string& outDir)
    : shadersDir(shadersDir), outDir(outDir)
{
	std::cout << "ShaderReloader initialized checking: " << this->shadersDir << std::endl;

	scanDirectory();
}

std::string ShaderReloader::readFileContent(const std::string& filepath)
{
	std::ifstream file(filepath);
	if (!file.is_open()) return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void ShaderReloader::scanDirectory()
{
	if (!std::filesystem::exists(shadersDir))
	{
		static bool warned = false;
		if (!warned)
		{
			std::cerr << "ShaderReloader Error: Directory does not exist! " << shadersDir << std::endl;
			warned = true;
		}
		return;
	}

	for (const auto& entry : std::filesystem::directory_iterator(shadersDir))
	{
		if (entry.is_regular_file() && entry.path().extension() == ".slang")
		{
			auto pathStr = entry.path().string();
			if (trackedShaders.find(pathStr) == trackedShaders.end())
			{
				ShaderInfo info;
				info.name = entry.path().stem().string();
				info.lastWriteTime = std::filesystem::last_write_time(entry);
				trackedShaders[pathStr] = info;
				std::cout << "ShaderReloader: Now tracking " << info.name << ".slang" << std::endl;
			}
		}
	}
}

bool ShaderReloader::compileShader(const std::string& slangPath, const std::string& slangContent,
                                   const std::string& outSpvPath)
{
	std::vector<std::string> entries;
	if (slangContent.find("vertMain") != std::string::npos) entries.push_back("vertMain");
	if (slangContent.find("fragMain") != std::string::npos) entries.push_back("fragMain");
	if (slangContent.find("computeMain") != std::string::npos) entries.push_back("computeMain");

	if (entries.empty())
	{
		std::cerr << "ShaderReloader: No known entry points found in " << slangPath << std::endl;
		return false;
	}

	std::string command =
	    "slangc \"" + slangPath + "\" -target spirv -profile spirv_1_4 -emit-spirv-directly -fvk-use-entrypoint-name";
	for (const auto& entry : entries)
	{
		command += " -entry " + entry;
	}
	command += " -o \"" + outSpvPath + "\"";

	std::cout << "ShaderReloader compiling: " << slangPath << std::endl;

	// We suppress output unless there's an error, std::system just runs it
	int result = std::system(command.c_str());
	if (result != 0)
	{
		std::cerr << "ShaderReloader compilation failed for: " << slangPath << std::endl;
		return false;
	}
	return true;
}

void ShaderReloader::update(PipelineManager& pManager, VulkanDevice& device)
{
	// Make sure the output directory exists
	if (!std::filesystem::exists(outDir))
	{
		std::filesystem::create_directories(outDir);
	}

	//scanDirectory(); // Can keep tracking new files on future inpl

	bool anyRebuilt = false;

	for (auto& [path, info] : trackedShaders)
	{
		if (!std::filesystem::exists(path)) continue;

		auto currentWriteTime = std::filesystem::last_write_time(path);
		if (currentWriteTime > info.lastWriteTime)
		{
			info.lastWriteTime = currentWriteTime;

			std::string content = readFileContent(path);
			if (content.empty()) continue;

			std::string outSpvPath = outDir + "/" + info.name + ".spv";

			if (compileShader(path, content, outSpvPath))
			{
				// Wait for the device to be idle before we rebuild pipelines.
				// We do it once here because multiple shaders could be changed, but usually it's one.
				device.device.waitIdle();

				// Find all pipelines that depend on this spv file
				std::vector<std::string> pipelinesToRebuild;
				for (auto& [pipelineName, builtDesc] : pManager.pipelines)
				{
					// Check if the shaderPath ends with the compiled spv path
					// e.g. "shaders/standard_forward.spv"
					if (builtDesc.desc.shaderPath == outDir + "/" + info.name + ".spv" ||
					    builtDesc.desc.shaderPath == outDir + "\\" + info.name + ".spv")
					{
						pipelinesToRebuild.push_back(pipelineName);
					}
				}

				for (const auto& pName : pipelinesToRebuild)
				{
					std::cout << "ShaderReloader rebuilding pipeline: " << pName << std::endl;
					pManager.rebuild(pName);
				}

				anyRebuilt = true;
				std::cout << "ShaderReloader successfully updated " << info.name << "\n";
			}
		}
	}
}
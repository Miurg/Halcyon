#include "GraphicsCore/ShaderReloader.hpp"

#include <algorithm>
#include <cctype>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <regex>
#include <sstream>

namespace
{
const std::regex vertexEntry(R"(\[\s*shader\s*\(\s*"vertex"\s*\)\s*\])");
const std::regex fragmentEntry(R"(\[\s*shader\s*\(\s*"fragment"\s*\)\s*\])");
const std::regex computeEntry(R"(\[\s*shader\s*\(\s*"compute"\s*\)\s*\])");
const std::regex includeDirective(R"regex(#\s*include\s*"([^"]+)")regex");
const std::regex importDirective(R"(\bimport\s+([A-Za-z_][A-Za-z0-9_.]*)\s*;)");

std::string normalizedPath(const std::filesystem::path& path)
{
	return std::filesystem::weakly_canonical(path).string();
}

std::vector<std::string> findEntryPoints(const std::string& content)
{
	std::vector<std::string> entryPoints;
	if (std::regex_search(content, vertexEntry)) entryPoints.push_back("vertMain");
	if (std::regex_search(content, fragmentEntry)) entryPoints.push_back("fragMain");
	if (std::regex_search(content, computeEntry)) entryPoints.push_back("computeMain");
	return entryPoints;
}
}

ShaderReloader::ShaderReloader(const std::string& shadersDir, const std::string& outDir)
    : shadersDir(shadersDir), outDir(outDir)
{
	std::cout << "ShaderReloader initialized checking: " << this->shadersDir << std::endl;
	scanDirectory();
}

std::string ShaderReloader::readFileContent(const std::string& filepath) const
{
	std::ifstream file(filepath);
	if (!file.is_open()) return "";
	std::stringstream buffer;
	buffer << file.rdbuf();
	return buffer.str();
}

void ShaderReloader::collectDependencies(const std::filesystem::path& sourcePath,
                                         std::unordered_set<std::string>& dependencies,
                                         std::unordered_set<std::string>& visited) const
{
	const std::string sourceKey = normalizedPath(sourcePath);
	if (!visited.insert(sourceKey).second) return;
	dependencies.insert(sourceKey);

	const std::string content = readFileContent(sourceKey);
	const std::filesystem::path includeDir = std::filesystem::path(shadersDir).parent_path() / "include";

	for (std::sregex_iterator it(content.begin(), content.end(), includeDirective), end; it != end; ++it)
	{
		const std::filesystem::path includeName = (*it)[1].str();
		const std::filesystem::path candidates[] = {
		    sourcePath.parent_path() / includeName,
		    includeDir / includeName,
		    std::filesystem::path(shadersDir) / includeName,
		};

		for (const auto& candidate : candidates)
		{
			if (!std::filesystem::exists(candidate)) continue;
			collectDependencies(candidate, dependencies, visited);
			break;
		}
	}

	for (std::sregex_iterator it(content.begin(), content.end(), importDirective), end; it != end; ++it)
	{
		std::string moduleName = (*it)[1].str();
		std::replace(moduleName.begin(), moduleName.end(), '.', '/');
		const std::filesystem::path modulePath = std::filesystem::path(shadersDir) / (moduleName + ".slang");
		if (std::filesystem::exists(modulePath)) collectDependencies(modulePath, dependencies, visited);
	}
}

void ShaderReloader::scanDirectory()
{
	trackedFiles.clear();
	entryShaders.clear();
	dependentShaders.clear();

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

	for (const auto& entry : std::filesystem::recursive_directory_iterator(shadersDir))
	{
		if (!entry.is_regular_file() || entry.path().extension() != ".slang") continue;

		const std::string content = readFileContent(entry.path().string());
		std::vector<std::string> entryPoints = findEntryPoints(content);
		if (entryPoints.empty()) continue;

		const std::string shaderKey = normalizedPath(entry.path());
		std::string outputName = entry.path().stem().string();
		std::transform(outputName.begin(), outputName.end(), outputName.begin(),
		               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
		entryShaders.emplace(shaderKey, ShaderInfo{entry.path(), std::move(outputName), std::move(entryPoints)});

		std::unordered_set<std::string> dependencies;
		std::unordered_set<std::string> visited;
		collectDependencies(entry.path(), dependencies, visited);
		for (const std::string& dependency : dependencies)
		{
			trackedFiles.try_emplace(dependency, std::filesystem::last_write_time(dependency));
			dependentShaders[dependency].insert(shaderKey);
		}
	}
}

bool ShaderReloader::compileShader(const ShaderInfo& shader, const std::string& outSpvPath)
{
	std::string command =
	    "slangc \"" + shader.sourcePath.string() +
	    "\" -target spirv -profile spirv_1_6 -emit-spirv-directly -fvk-use-entrypoint-name"
	    " -capability SPV_KHR_non_semantic_info+SPV_GOOGLE_user_type+spvDerivativeControl+spvImageQuery+"
	    "spvImageGatherExtended+spvSparseResidency+spvMinLod+spvFragmentFullyCoveredEXT+spvShaderNonUniformEXT"
	    " -I \"" + (std::filesystem::path(shadersDir).parent_path() / "include").string() +
	    "\" -I \"" + shadersDir + "\"";
	for (const auto& entry : shader.entryPoints)
	{
		command += " -entry " + entry;
	}
	command += " -o \"" + outSpvPath + "\"";

	std::cout << "ShaderReloader compiling: " << shader.sourcePath.string() << std::endl;
	int result = std::system(command.c_str());
	if (result != 0)
	{
		std::cerr << "ShaderReloader compilation failed for: " << shader.sourcePath.string() << std::endl;
		return false;
	}
	return true;
}

void ShaderReloader::update(PipelineManager& pipelineManager, VulkanDevice& device)
{
	const auto now = std::chrono::steady_clock::now();
	if (now < nextPollTime) return;
	nextPollTime = now + std::chrono::milliseconds(100);

	std::unordered_set<std::string> dirtyShaders;
	for (const auto& [path, lastWriteTime] : trackedFiles)
	{
		std::error_code error;
		const auto currentWriteTime = std::filesystem::last_write_time(path, error);
		if (!error && currentWriteTime == lastWriteTime) continue;

		if (auto dependents = dependentShaders.find(path); dependents != dependentShaders.end())
		{
			dirtyShaders.insert(dependents->second.begin(), dependents->second.end());
		}
	}

	if (dirtyShaders.empty()) return;
	scanDirectory();

	if (!std::filesystem::exists(outDir)) std::filesystem::create_directories(outDir);

	std::unordered_set<std::string> rebuiltShaders;
	for (const std::string& shaderPath : dirtyShaders)
	{
		auto shaderIt = entryShaders.find(shaderPath);
		if (shaderIt == entryShaders.end()) continue;

		const ShaderInfo& shader = shaderIt->second;
		const std::string outputFile = shader.outputName + ".spv";
		const std::string outSpvPath = (std::filesystem::path(outDir) / outputFile).string();
		if (compileShader(shader, outSpvPath)) rebuiltShaders.insert(outputFile);
	}

	if (rebuiltShaders.empty()) return;

	device.device.waitIdle();
	for (const auto& [pipelineName, builtDesc] : pipelineManager.pipelines)
	{
		if (!rebuiltShaders.contains(builtDesc.desc.shaderPath)) continue;
		std::cout << "ShaderReloader rebuilding pipeline: " << pipelineName << std::endl;
		pipelineManager.rebuild(pipelineName);
	}
}

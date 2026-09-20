#pragma once

#include <chrono>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

class FileInspector
{
public:
	explicit FileInspector(std::filesystem::path rootDirectory = "assets");
	std::optional<std::filesystem::path> draw();
	void setError(std::string message);

private:
	struct Entry
	{
		std::filesystem::path path;
		std::string label;
		std::string searchName;
		bool directory;
	};

	std::filesystem::path rootDirectory;
	std::filesystem::path currentDirectory;
	std::filesystem::path selectedPath;
	std::filesystem::path contextPath;
	std::string directoryLabel;
	std::vector<Entry> entries;
	std::vector<int> visibleEntries;
	char search[128]{};
	std::string directoryError;
	std::string actionError;
	std::chrono::steady_clock::time_point nextCheck{};
	std::filesystem::file_time_type directoryWriteTime{};
	bool hasSnapshot = false;
	bool resetScroll = false;

	void refresh(bool force = false);
	void navigate(const std::filesystem::path& directory);
	void filterEntries();
	bool containsFile(const std::filesystem::path& path) const;
};

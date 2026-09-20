#ifdef HALCYON_DEV_TOOLS

#include "FileInspector.hpp"

#include <imgui.h>
#include <algorithm>
#include <cctype>
#include <system_error>
#include <utility>

namespace
{
std::string pathText(const std::filesystem::path& path)
{
	const auto text = path.u8string();
	return {text.begin(), text.end()};
}

std::string lowerCase(std::string text)
{
	std::transform(text.begin(), text.end(), text.begin(),
	               [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
	return text;
}

bool isModel(const std::filesystem::path& path)
{
	const auto extension = lowerCase(pathText(path.extension()));
	return extension == ".gltf" || extension == ".glb";
}
}

FileInspector::FileInspector(std::filesystem::path rootDirectory)
    : rootDirectory(std::filesystem::absolute(rootDirectory).lexically_normal()),
      currentDirectory(this->rootDirectory), directoryLabel(pathText(currentDirectory))
{
}

void FileInspector::setError(std::string message)
{
	actionError = std::move(message);
}

void FileInspector::filterEntries()
{
	const auto query = lowerCase(search);
	visibleEntries.clear();
	for (size_t i = 0; i < entries.size(); ++i)
	{
		if (entries[i].searchName.find(query) != std::string::npos)
			visibleEntries.push_back(static_cast<int>(i));
	}
}

bool FileInspector::containsFile(const std::filesystem::path& path) const
{
	return std::any_of(entries.begin(), entries.end(),
	                   [&](const Entry& entry) { return !entry.directory && entry.path == path; });
}

void FileInspector::navigate(const std::filesystem::path& directory)
{
	currentDirectory = directory;
	directoryLabel = pathText(currentDirectory);
	selectedPath.clear();
	contextPath.clear();
	search[0] = '\0';
	hasSnapshot = false;
	resetScroll = true;
	refresh(true);
}

void FileInspector::refresh(bool force)
{
	// Keep filesystem access off the per-frame path and reuse unchanged directory snapshots.
	const auto now = std::chrono::steady_clock::now();
	if (!force && now < nextCheck) return;
	nextCheck = now + std::chrono::milliseconds(500);

	std::error_code error;
	auto writeTime = std::filesystem::last_write_time(currentDirectory, error);
	while (currentDirectory != rootDirectory &&
	       (error == std::errc::no_such_file_or_directory || error == std::errc::not_a_directory))
	{
		currentDirectory = currentDirectory.parent_path();
		selectedPath.clear();
		contextPath.clear();
		search[0] = '\0';
		directoryLabel = pathText(currentDirectory);
		hasSnapshot = false;
		resetScroll = true;
		writeTime = std::filesystem::last_write_time(currentDirectory, error);
	}
	if (!force && !error && hasSnapshot && writeTime == directoryWriteTime) return;

	std::vector<Entry> snapshot;
	if (!error)
	{
		for (std::filesystem::directory_iterator it(currentDirectory, error), end; !error && it != end;
		     it.increment(error))
		{
			std::error_code entryError;
			const bool directory = it->is_directory(entryError);
			if (entryError) continue;
			if (!directory && (!it->is_regular_file(entryError) || entryError)) continue;
			const auto name = pathText(it->path().filename());
			snapshot.push_back({it->path(), directory ? "[Folder] " + name : name, lowerCase(name), directory});
		}
	}

	hasSnapshot = !error;
	directoryError = error ? "Cannot read folder: " + error.message() : std::string{};
	if (error) snapshot.clear();
	std::sort(snapshot.begin(), snapshot.end(),
	          [](const Entry& a, const Entry& b)
	          {
		          if (a.directory != b.directory) return a.directory;
		          if (a.searchName != b.searchName) return a.searchName < b.searchName;
		          return a.path < b.path;
	          });
	entries = std::move(snapshot);
	directoryWriteTime = writeTime;
	if (std::none_of(entries.begin(), entries.end(),
	                 [&](const Entry& entry) { return entry.path == selectedPath; }))
		selectedPath.clear();
	filterEntries();
}

std::optional<std::filesystem::path> FileInspector::draw()
{
	ImGui::SetNextWindowSize(ImVec2(460, 420), ImGuiCond_FirstUseEver);
	if (!ImGui::Begin("File Inspector"))
	{
		ImGui::End();
		return std::nullopt;
	}

	refresh();
	ImGui::BeginDisabled(currentDirectory == rootDirectory);
	if (ImGui::Button("Up")) navigate(currentDirectory.parent_path());
	ImGui::SameLine();
	if (ImGui::Button("Root")) navigate(rootDirectory);
	ImGui::EndDisabled();
	ImGui::SameLine();
	if (ImGui::Button("Refresh")) refresh(true);
	ImGui::TextWrapped("%s", directoryLabel.c_str());
	ImGui::SetNextItemWidth(-1.0f);
	if (ImGui::InputTextWithHint("##SearchFiles", "Search this folder...", search, sizeof(search)))
	{
		filterEntries();
		resetScroll = true;
	}
	ImGui::TextDisabled("Double-click folders, right-click files");
	if (!directoryError.empty()) ImGui::TextWrapped("%s", directoryError.c_str());
	if (!actionError.empty())
	{
		ImGui::TextWrapped("Action failed: %s", actionError.c_str());
		if (ImGui::SmallButton("Dismiss")) actionError.clear();
	}

	std::optional<std::filesystem::path> directoryToOpen;
	bool openContext = false;
	if (ImGui::BeginChild("Entries", ImVec2(0, 0), ImGuiChildFlags_Borders))
	{
		if (resetScroll)
		{
			ImGui::SetScrollY(0.0f);
			resetScroll = false;
		}
		if (visibleEntries.empty() && directoryError.empty()) ImGui::TextDisabled("No files or folders found.");
		ImGuiListClipper clipper;
		clipper.Begin(static_cast<int>(visibleEntries.size()));
		while (clipper.Step())
		{
			for (int i = clipper.DisplayStart; i < clipper.DisplayEnd; ++i)
			{
				const auto& entry = entries[visibleEntries[i]];
				ImGui::PushID(entry.label.c_str());
				const auto position = ImGui::GetCursorScreenPos();
				if (ImGui::Selectable("##Entry", selectedPath == entry.path, ImGuiSelectableFlags_AllowDoubleClick,
				                      ImVec2(0, ImGui::GetTextLineHeight())))
				{
					selectedPath = entry.path;
					if (entry.directory && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left))
						directoryToOpen = entry.path;
				}
				if (!entry.directory && ImGui::IsItemHovered() && ImGui::IsMouseReleased(ImGuiMouseButton_Right))
				{
					selectedPath = contextPath = entry.path;
					openContext = true;
				}
				ImGui::GetWindowDrawList()->AddText(position, ImGui::GetColorU32(ImGuiCol_Text), entry.label.c_str());
				ImGui::PopID();
			}
		}
	}
	ImGui::EndChild();

	// Keep the popup independent of clipped rows and snapshot reordering.
	std::optional<std::filesystem::path> requestedPath;
	if (openContext) ImGui::OpenPopup("FileActions");
	if (ImGui::BeginPopup("FileActions"))
	{
		if (!containsFile(contextPath))
		{
			ImGui::CloseCurrentPopup();
		}
		else
		{
			ImGui::TextUnformatted(pathText(contextPath.filename()).c_str());
			ImGui::Separator();
			if (ImGui::MenuItem("Copy path")) ImGui::SetClipboardText(pathText(contextPath).c_str());
			if (isModel(contextPath) && ImGui::MenuItem("Add model to scene"))
			{
				std::error_code error;
				if (std::filesystem::is_regular_file(contextPath, error))
				{
					requestedPath = contextPath;
					actionError.clear();
				}
				else
				{
					actionError = error ? error.message() : "The file is no longer available.";
					refresh(true);
				}
			}
		}
		ImGui::EndPopup();
	}
	if (directoryToOpen) navigate(*directoryToOpen);
	ImGui::End();
	return requestedPath;
}

#endif

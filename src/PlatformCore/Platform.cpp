#include "PlatformCore/Platform.hpp"

#include <filesystem>
#include <cstdlib>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>
#elif defined(__linux__)
#include <system_error>
#endif

namespace Platform
{
	std::string executableDir()
	{
#if defined(_WIN32)
		wchar_t buf[MAX_PATH];
		DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
		if (len == 0 || len == MAX_PATH) return {};
		return std::filesystem::path(std::wstring(buf, len)).parent_path().string();
#elif defined(__linux__)
		std::error_code ec;
		std::filesystem::path exe = std::filesystem::read_symlink("/proc/self/exe", ec);
		if (ec) return {};
		return exe.parent_path().string();
#else
		return {};
#endif
	}

	std::string getEnv(const char* name)
	{
#if defined(_WIN32)
		char* value = nullptr;
		size_t len = 0;
		std::string result;
		if (_dupenv_s(&value, &len, name) == 0 && value)
		{
			result = value;
			free(value);
		}
		return result;
#else
		const char* value = std::getenv(name);
		return value ? value : std::string{};
#endif
	}
}

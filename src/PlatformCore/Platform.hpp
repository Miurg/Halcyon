#pragma once

#include <string>

namespace Platform
{
	std::string executableDir();
	std::string getEnv(const char* name);
}

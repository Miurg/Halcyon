#pragma once
#include <string>
#include <cstring>

struct NameComponent
{
	char name[256];

	NameComponent()
	{
		std::strncpy(name, "Unnamed Entity", sizeof(name));
		name[sizeof(name) - 1] = '\0';
	}

	NameComponent(const std::string& entityName)
	{
		std::strncpy(name, entityName.c_str(), sizeof(name));
		name[sizeof(name) - 1] = '\0';
	}

	NameComponent(const char* entityName)
	{
		std::strncpy(name, entityName, sizeof(name));
		name[sizeof(name) - 1] = '\0';
	}
};

#pragma once
#include <Orhescyon/GeneralManager.hpp>

using Orhescyon::GeneralManager;

class PlaceholdersInit
{
public:
	static void initPlaceholders(GeneralManager& gm);
	static void initAfterCorePlaceholders(GeneralManager& gm);
};
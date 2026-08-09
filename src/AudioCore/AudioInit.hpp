#pragma once
#include <Orhescyon/GeneralManager.hpp>

class AudioInit
{
public:
	static void Run(Orhescyon::GeneralManager& gm);

private:
	static void coreInit(Orhescyon::GeneralManager& gm);
	static void initAudio(Orhescyon::GeneralManager& gm);
};

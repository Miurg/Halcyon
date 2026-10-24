#pragma once
#include <Orhescyon/GeneralManager.hpp>

class JobRuntime;

class PhysicsInit
{
public:
	static void Run(Orhescyon::GeneralManager& gm, JobRuntime& jobRuntime);

private:
	static void coreInit(Orhescyon::GeneralManager& gm);
	static void initPhysics(Orhescyon::GeneralManager& gm, JobRuntime& jobRuntime);
};

#pragma once

#include <Orhescyon/GeneralManager.hpp>

class JobRuntime;

class JobsInit
{
public:
	static void Run(Orhescyon::GeneralManager& gm, JobRuntime& jobRuntime);

private:
	static void initJobs(Orhescyon::GeneralManager& gm, JobRuntime& jobRuntime);
};

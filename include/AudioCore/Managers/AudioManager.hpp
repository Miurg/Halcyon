#pragma once

#include <string>
#include "HalcyonExport.hpp"

class HALCYON_API AudioManager
{
public:
	AudioManager();
	~AudioManager();

	void play(const std::string& filepath);

private:
	struct Impl;
	Impl* pImpl;
};
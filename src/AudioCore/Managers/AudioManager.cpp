#define MINIAUDIO_IMPLEMENTATION
#include "AudioCore/Managers/AudioManager.hpp"
#include "../miniaudio.h"

struct AudioManager::Impl
{
	ma_engine engine;
	ma_result result;
};

AudioManager::AudioManager()
{
	pImpl = new Impl();
	pImpl->result = ma_engine_init(NULL, &pImpl->engine);
	if (pImpl->result != MA_SUCCESS)
	{
		std::cerr << "MA: Error, on create MA: (" << pImpl->result << ")\n";
	}
}

AudioManager::~AudioManager()
{
	ma_engine_uninit(&pImpl->engine);
	delete pImpl;
}

void AudioManager::play(const std::string& filepath)
{
	pImpl->result = ma_engine_play_sound(&pImpl->engine, filepath.c_str(), NULL);

	if (pImpl->result != MA_SUCCESS)
	{
		std::cerr << "MA: Error, on play: (" << pImpl->result << ")\n";
	}
}
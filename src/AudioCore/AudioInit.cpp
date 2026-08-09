#include "AudioInit.hpp"

#include <iostream>
#include <thread>
#include <Orhescyon/GeneralManager.hpp>

#include "DeletionQueueComponent.hpp"
#include "DeletionQueueContext.hpp"

#include "AudioCore/Managers/AudioManager.hpp"
#include "AudioCore/Components/AudioManagerComponent.hpp"
#include "AudioCore/AudioContexts.hpp"

#pragma region Run
    void AudioInit::Run(Orhescyon::GeneralManager& gm)
{
#ifdef _DEBUG
	std::cout << "AUDIOSINIT::RUN::Start init" << std::endl;
#endif //_DEBUG

	coreInit(gm);
	initAudio(gm);

#ifdef _DEBUG
	std::cout << "AUDIOINIT::RUN::Succes!" << std::endl;
#endif //_DEBUG
}
#pragma endregion

#pragma region coreInit
void AudioInit::coreInit(Orhescyon::GeneralManager& gm)
{
	gm.registerSystemManager("audio");
}
#pragma endregion

#pragma region initPhysics
void AudioInit::initAudio(Orhescyon::GeneralManager& gm)
{
	DeletionQueue* dq = gm.getContextComponent<DeletionQueueContext, DeletionQueueComponent>()->queue;

	Orhescyon::Entity audioManagerEntity = gm.createEntity();
	AudioManager* audioManager = new AudioManager();
	gm.addComponent<AudioManagerComponent>(audioManagerEntity, audioManager);
	gm.registerContext<AudioManagerContext>(audioManagerEntity);
	dq->push_function([audioManager]() { delete audioManager; });
}
#pragma endregion

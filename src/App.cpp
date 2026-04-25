#include "App.hpp"

#include <iostream>
#define GLM_FORCE_SIMD_AVX2
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#define TINYGLTF_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#define VMA_IMPLEMENTATION
#include "vk_mem_alloc.h"
#include "CoreInit.hpp"
#include "Game/GameInit.hpp"
#include "Graphics/GraphicsInit/GraphicsInit.hpp"
#include "Cleanup.hpp"
#include "MainLoop.hpp"
#include <exception>
#include <Orhescyon/GeneralManager.hpp>
#include "PhysicsCore/PhysicsInit.hpp"
App::App() {}
using Orhescyon::GeneralManager;
int App::run()
{
	GeneralManager gm;
	try
	{
		CoreInit::Run(gm);
		GraphicsInit::Run(gm);
		PhysicsInit::Run(gm);
		GameInit::gameInitStart(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::INIT::Exception: " << e.what() << std::endl;
		Cleanup::cleanup(gm);
		return EXIT_FAILURE;
	}
	
	try
	{
		MainLoop::startLoop(gm);
	}
	catch (const std::exception& e)
	{
		std::cerr << "ERROR::APP::MAINLOOP::Exception: " << e.what() << std::endl;
		Cleanup::cleanup(gm);
		return EXIT_FAILURE;
	}
	Cleanup::cleanup(gm);
	return EXIT_SUCCESS;
}

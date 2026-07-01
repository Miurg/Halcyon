#include <Halcyon.hpp>
#include "Game/GameInit.hpp"

int main()
{
	GameInit game;
	return App::create().addStartUp(game).run();
}
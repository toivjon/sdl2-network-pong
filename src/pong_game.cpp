#include "pong_game.h"

#include <iostream>

using namespace pong;

Game::Game(int argc, char** argv)
{
    std::cout << "Hello Pong: " << argc << " -- " << argv << std::endl;
    // ... RAII initialization ...
}

Game::~Game()
{
    // ... RAII shutdown ...
}

void Game::start()
{
    // ... start the game ...
}
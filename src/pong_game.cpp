#include "pong_game.h"

#include <iostream>
#include <SDL/SDL.h>

using namespace pong;

Game::Game(int argc, char** argv)
{
  // TODO ... handle command line arguments ...
  std::cout << "Hello Pong: " << argc << " -- " << argv << std::endl;

  SDL_Log("Constructing a Game instance....");

  // initialize SDL along with the subsystems.
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    SDL_Log("Unable to initialize SDL: %s", SDL_GetError());
    throw "Unable to initialize SDL!";
  }

  SDL_Log("Constructing a Game instance completed.");
}

Game::~Game()
{
  SDL_Log("Destructing a Game instance...");

  // release all reserved resources.
  SDL_Quit();

  SDL_Log("Destructing a Game instance completed.");
}

void Game::start()
{
  // ... start the game ...
}

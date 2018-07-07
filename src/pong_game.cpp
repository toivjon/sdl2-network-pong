#include "pong_game.h"

#include <iostream>
#include <iterator>
#include <SDL/SDL.h>
#include <string>
#include <vector>

using namespace pong;

Game::Game(int argc, char** argv)
{
  SDL_Log("Constructing a Game instance....");

  // process the provided command line arguments.
  for (int i = 1; i < argc; i++) {
    // ... handle?
    std::string arg(argv[i]);
  }

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

#include "game.h"

#include <cstdio>
#include <SDL/SDL.h>
#include <stdexcept>

using namespace pong;

Game::Game(int argc, char* argv[])
{
  printf("%d %s", argc, argv[0]);

  // initialize SDL along with all subsystems.
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init: %s\n", SDL_GetError());
    throw std::runtime_error("Unable to initialize SDL.");
  }
}

Game::~Game()
{
  SDL_Quit();
}

void Game::run()
{
  // ...
}

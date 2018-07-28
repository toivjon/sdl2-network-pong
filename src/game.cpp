#include "game.h"

#include <cstdio>
#include <SDL/SDL.h>
#include <stdexcept>

using namespace pong;

Game::Game(int argc, char* argv[]) : mWindow(NULL)
{
  printf("%d %s", argc, argv[0]);

  // initialize SDL along with all subsystems.
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init: %s\n", SDL_GetError());
    throw std::runtime_error("Unable to initialize SDL.");
  }

  // construct the main SDL window for the game.
  mWindow = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
  if (mWindow == NULL) {
    printf("SDL_CreateWindow: %s\n", SDL_GetError());
    throw std::runtime_error("Unable to create the main SDL window.");
  }
}

Game::~Game()
{
  SDL_DestroyWindow(mWindow);
  SDL_Quit();
}

void Game::run()
{
  // ...
}

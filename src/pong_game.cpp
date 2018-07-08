#include "pong_game.h"

#include <iterator>
#include <SDL/SDL.h>
#include <string>
#include <vector>

using namespace pong;

Game::Game(int argc, char** argv) : mWidth(800), mHeight(600), mWindow(nullptr), mRenderer(nullptr)
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
  SDL_DestroyRenderer(mRenderer);
  SDL_DestroyWindow(mWindow);
  SDL_Quit();

  SDL_Log("Destructing a Game instance completed.");
}

void Game::start()
{
  SDL_Log("Starting the game...");

  // create the main window for the application.
  mWindow = SDL_CreateWindow("SDL2 - Network Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, mWidth, mHeight, SDL_WINDOW_SHOWN);
  if (mWindow == nullptr) {
    SDL_Log("Unable to create SDL window: %s", SDL_GetError());
    throw "Unable to create SDL window!";
  }

  // create a new renderer for the main window of the application.
  mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  if (mRenderer == nullptr) {
    SDL_Log("Unable to create SDL renderer: %s", SDL_GetError());
    throw "Unable to create SDL renderer!";
  }

  // start the main loop of the application.
  auto isRunning = true;
  SDL_Event event;
  while (isRunning) {
    // poll and handle events from the SDL framework.
    while (SDL_PollEvent(&event)) {
      switch (event.type) {
        case SDL_QUIT:
          isRunning = false;
          break;
      }
    }

    // clear the contents of the currently active buffer.
    SDL_SetRenderDrawColor(mRenderer, 0x00, 0x00, 0x00, 0xff);
    SDL_RenderClear(mRenderer);

    // TODO ... add draw calls here ...

    // swap and present the current buffer on the screen.
    SDL_RenderPresent(mRenderer);
  }
}

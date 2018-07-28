#include "network_system.h"

#include <cstdio>
#include <SDL/SDL_net.h>
#include <stdexcept>

using namespace pong;

NetworkSystem::NetworkSystem(Game& game) : mGame(game)
{
  // initialize the SDLNet system.
  if (SDLNet_Init() != 0) {
    printf("SDLNet_Init: %s\n", SDLNet_GetError());
    throw std::runtime_error("Failed to initialize SDLNet!");
  }
}

NetworkSystem::~NetworkSystem()
{
  SDLNet_Quit();
}

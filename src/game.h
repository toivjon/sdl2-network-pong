#ifndef PONG_GAME_H
#define PONG_GAME_H

#include "network_system.h"

struct SDL_Window;
struct SDL_Renderer;

namespace pong
{
  class Game final
  {
    public:
      // ======================================================================
      // Construct a new game with the provided command line arguments. This is
      // the key point to construct a new Pong instance. Command line arguments
      // are used to configure the game e.g. to act as a server or client.
      //
      // @param argc The amount of arguments.
      // @param argv The array of arguments.
      // ======================================================================
      Game(int argc, char* argv[]);

      ~Game();

      // ======================================================================
      // Start and run the game by launching the main loop of the application.
      // This function will return after the game has been successfully closed.
      // ======================================================================
      void run();
    private:
      SDL_Window*   mWindow;
      SDL_Renderer* mRenderer;
      NetworkSystem mNetworkSystem;
  };
}

#endif

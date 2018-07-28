#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include <cstdio>

int main(int argc, char* argv[]) {
  printf("%d %s", argc, argv[0]);

  // initialize the core SDL framework.
  auto result = SDL_Init(SDL_INIT_VIDEO);
  SDL_assert(result == 0);

  // initialize the SDL network framework.
  result = SDLNet_Init();
  SDL_assert(result == 0);

  // create the main window for the application.
  auto window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN);
  SDL_assert(window != NULL);

  // create the renderer for the main window.
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_assert(renderer != NULL);

  // start the main loop.
  auto isRunning = true;
  SDL_Event event;
  while (isRunning) {
    while(SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          isRunning = false;
          break;
      }

      // clear the backbuffer with the black color.
      SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
      SDL_RenderClear(renderer);

      // TODO draw stuff...

      // swap backbuffer to front and vice versa.
      SDL_RenderPresent(renderer);
    }
  }

  // release resources.
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDLNet_Quit();
  SDL_Quit();

  return 0;
}

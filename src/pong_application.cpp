#include "pong_application.h"

#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include <cstdio>
#include <cstdlib>

using namespace pong;

Application::Application(int maxsockets)
  : mWindow(NULL),
    mRenderer(NULL),
    mSocketSet(NULL),
    mRunning(false),
    mTopWall({0, 0, 800, 20}),
    mBottomWall({0, 580, 800, 20}),
    mCenterLine({390, 0, 20, 600}),
    mLeftPaddle({30, 250, 20, 100}),
    mRightPaddle({750, 250, 20, 100}),
    mBall({390, 290, 20, 20})
{
  // initialize SDL along with the subsystems.
  if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
    printf("SDL_Init: %s", SDL_GetError());
    exit(-1);
  }

  // initialize the SDL net system.
  if (SDLNet_Init() != 0) {
    printf("SDLNet_Init: %s", SDLNet_GetError());
    exit(-1);
  }

  // create a main window for the application.
  if ((mWindow = SDL_CreateWindow("SDL2 - Network Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_SHOWN)) == NULL) {
    printf("SDL_CreateWindow: %s", SDL_GetError());
    exit(-1);
  }

  // create a renderer for the main window of the application.
  if ((mRenderer = SDL_CreateRenderer(mWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC)) == NULL) {
    printf("SDL_CreateRenderer: %s", SDL_GetError());
    exit(-1);
  }

  // allocate a new socket set for the connection sockets.
  if ((mSocketSet = SDLNet_AllocSocketSet(maxsockets)) == NULL) {
    printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
    exit(-1);
  }
}

Application::~Application()
{
  SDLNet_FreeSocketSet(mSocketSet);
  SDL_DestroyRenderer(mRenderer);
  SDL_DestroyWindow(mWindow);
  SDLNet_Quit();
  SDL_Quit();
}

void Application::render()
{
  // clear the contents of the currently active buffer.
  SDL_SetRenderDrawColor(mRenderer, 0x00, 0x00, 0x00, 0xff);
  SDL_RenderClear(mRenderer);

  // render all static objects in the scene.
  mTopWall.render(*mRenderer);
  mBottomWall.render(*mRenderer);
  mCenterLine.render(*mRenderer);

  // render all dynamic objects in the scene.
  mLeftPaddle.render(*mRenderer, 0l);
  mRightPaddle.render(*mRenderer, 0l);
  mBall.render(*mRenderer, 0l);

  // swap and present the current buffer on the screen.
  SDL_RenderPresent(mRenderer);
}

void Application::tickSDL()
{
  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    switch (event.type) {
      case SDL_QUIT:
        setRunning(false);
        break;
      case SDL_KEYUP:
        onKeyUp(event.key);
        break;
      case SDL_KEYDOWN:
        onKeyDown(event.key);
        break;
    }
  }
}

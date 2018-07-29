#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include <cstdio>

int main(int argc, char* argv[]) {
  // parse command line arguments.
  auto isServer = (argc == 1);
  auto host = (isServer ? NULL : argv[1]);

  // initialize the core SDL framework.
  auto result = SDL_Init(SDL_INIT_VIDEO);
  SDL_assert(result == 0);

  // initialize the SDL network framework.
  result = SDLNet_Init();
  SDL_assert(result == 0);

  // create the main window for the application.
  int resolutionX = 800;
  int resolutionY = 600;
  auto window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, resolutionX, resolutionY, SDL_WINDOW_SHOWN);
  SDL_assert(window != NULL);

  // create the renderer for the main window.
  auto renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
  SDL_assert(renderer != NULL);

  // resolve the requires host address for the initial socket.
  IPaddress ip;
  result = SDLNet_ResolveHost(&ip, host, 6666);
  SDL_assert(result == 0);

  // create the initial socket which is either client or server socket.
  auto socket = SDLNet_TCP_Open(&ip);
  SDL_assert(socket != NULL);

  // allocate a socket set for listening the socket activity.
  auto socketSet = SDLNet_AllocSocketSet(isServer ? 2 : 1);
  SDL_assert(socketSet != NULL);

  // add the initial socket into the socket set.
  result = SDLNet_TCP_AddSocket(socketSet, socket);
  SDL_assert(result != -1);

  // create the set of game objects.
  auto boxWidth = (resolutionY / 30);
  auto edgeOffset = (resolutionY / 20);
  auto paddleHeight = (resolutionY / 6);
  SDL_Rect topWall = { 0, 0, resolutionX, boxWidth};
  SDL_Rect bottomWall = { 0, (resolutionY - boxWidth), resolutionX, boxWidth };
  SDL_Rect leftPaddle = { edgeOffset, ((resolutionY / 2) - (paddleHeight / 2)), boxWidth, paddleHeight};
  SDL_Rect rightPaddle = { (resolutionX - edgeOffset - boxWidth), ((resolutionY / 2) - (paddleHeight / 2)), boxWidth, paddleHeight};
  SDL_Rect ball = { ((resolutionX / 2) - (boxWidth / 2)), ((resolutionY / 2) - (boxWidth / 2)), boxWidth, boxWidth };
  SDL_Rect leftGoal = { -1000, 0, (1000 - boxWidth), resolutionY };
  SDL_Rect rightGoal = { resolutionX + boxWidth, 0, 1000, resolutionY };
  SDL_Rect centerLine[15];
  for (auto i = 0; i < 15; i++) {
    auto y = static_cast<int>(boxWidth + (i * 1.93f * boxWidth));
    centerLine[i] = { ((resolutionX / 2) - (boxWidth / 2)), y, boxWidth, boxWidth };
  }

  // variables used within the ingame logic.
  const auto paddleVelocity = (resolutionX / 100);
  auto paddleDirection = 0;

  // start the main loop.
  auto isRunning = true;
  SDL_Event event;
  while (isRunning) {
    while(SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          isRunning = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              paddleDirection = -1;
              break;
            case SDLK_DOWN:
              paddleDirection = 1;
              break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              if (paddleDirection == -1) {
                paddleDirection = 0;
              }
              break;
            case SDLK_DOWN:
              if (paddleDirection == 1) {
                paddleDirection = 0;
              }
              break;
          }
          break;
      }
    }

    // move the controlled paddle when required.
    if (paddleDirection != 0) {
      auto movement = paddleVelocity * paddleDirection;
      if (isServer) {
        leftPaddle.y += movement;
      } else {
        rightPaddle.y += movement;
      }
    }

    // check that the left paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&leftPaddle, &topWall)) {
      leftPaddle.y = (topWall.y + topWall.h);
    } else if (SDL_HasIntersection(&leftPaddle, &bottomWall)) {
      leftPaddle.y = (bottomWall.y - paddleHeight);
    }

    // check that the right paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&rightPaddle, &topWall)) {
      rightPaddle.y = (topWall.y + topWall.h);
    } else if (SDL_HasIntersection(&rightPaddle, &bottomWall)) {
      rightPaddle.y = (bottomWall.y - paddleHeight);
    }

    // perform collision detection logics for the ball.
    if (SDL_HasIntersection(&ball, &leftGoal)) {
      // TODO give a score to right player.
      // TODO reset ball & paddles to initial positions.
      // TODO randomize ball direction.
      // TODO reset ball velocity.
    } else if (SDL_HasIntersection(&ball, &rightGoal)) {
      // TODO give a score to left player.
      // TODO reset ball & paddles to initial positions.
      // TODO randomize ball direction.
      // TODO reset ball velocity.
    } else if (SDL_HasIntersection(&ball, &leftPaddle)) {
      // TODO inverse ball x- and y-direction.
      // TODO apply a small increment to ball velocity.
    } else if (SDL_HasIntersection(&ball, &rightPaddle)) {
      // TODO inverse ball x- and y-direction.
      // TODO apply a small increment to ball velocity.
    } else if (SDL_HasIntersection(&ball, &topWall)) {
      // TODO inverse ball y-direction.
    } else if (SDL_HasIntersection(&ball, &bottomWall)) {
      // TODO inverse ball y-direction.
    }

    // clear the backbuffer with the black color.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);

    // render all visible game objects on the backbuffer.
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRect(renderer, &topWall);
    SDL_RenderFillRect(renderer, &bottomWall);
    SDL_RenderFillRect(renderer, &leftPaddle);
    SDL_RenderFillRect(renderer, &rightPaddle);
    SDL_RenderFillRect(renderer, &ball);
    SDL_RenderFillRects(renderer, &centerLine[0], 15);

    // swap backbuffer to front and vice versa.
    SDL_RenderPresent(renderer);
  }

  // release resources.
  SDLNet_TCP_DelSocket(socketSet, socket);
  SDLNet_FreeSocketSet(socketSet);
  SDLNet_TCP_Close(socket);
  SDL_DestroyRenderer(renderer);
  SDL_DestroyWindow(window);
  SDLNet_Quit();
  SDL_Quit();

  return 0;
}

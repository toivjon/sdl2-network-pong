#ifndef PONG_GAME_H
#define PONG_GAME_H

// forward declarations.
struct SDL_Window;
struct SDL_Renderer;

namespace pong
{
  class Game final
  {
    public:
      Game(int argc, char** argv);
      ~Game();

      void start();
    private:
      int           mWidth;
      int           mHeight;
      SDL_Window*   mWindow;
      SDL_Renderer* mRenderer;
  };
}

#endif

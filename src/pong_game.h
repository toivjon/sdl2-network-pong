#ifndef PONG_GAME_H
#define PONG_GAME_H

namespace pong
{
  class Game final
  {
    public:
      Game(int argc, char** argv);
      ~Game();

      void start();
  };
}

#endif

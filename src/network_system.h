#ifndef NETWORK_SYSTEM_H
#define NETWORK_SYSTEM_H

struct _SDLNet_SocketSet;

namespace pong
{
  class Game;
  class NetworkSystem final
  {
    public:
      // ======================================================================
      // Construct a new network system for the provided game instance. This
      // function must be called with a valid reference to an existing game so
      // this system may use the injected instance as the base game structure.
      //
      // @param game A reference to target game.
      // ======================================================================
      NetworkSystem(Game& game);

      ~NetworkSystem();
    private:
      Game&              mGame;
      _SDLNet_SocketSet* mSocketSet;
  };
}

#endif

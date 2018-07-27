#ifndef PONG_SERVER_H
#define PONG_SERVER_H

#include "pong_application.h"

// forward declarations.
struct _TCPsocket;

namespace pong
{
  class Server final : public Application
  {
    public:
      Server(Server&) = delete;
      Server(Server&&) = delete;

      Server& operator=(Server&) = delete;
      Server& operator=(Server&&) = delete;

      Server();

      ~Server();

      void start();

      void onKeyUp(const SDL_KeyboardEvent& event) override;
      void onKeyDown(const SDL_KeyboardEvent& event) override;
    private:
      void tickSockets();
    private:
      _TCPsocket* mServerSocket;
      _TCPsocket* mClientSocket;
  };
}

#endif

#ifndef PONG_CLIENT_H
#define PONG_CLIENT_H

#include "pong_application.h"

#include <string>

// forward declarations.
struct _TCPsocket;

namespace pong
{
  class Client final : public Application
  {
    public:
      Client() = delete;
      Client(Client&) = delete;
      Client(Client&&) = delete;

      Client& operator=(Client&) = delete;
      Client& operator=(Client&&) = delete;

      Client(const std::string& host);

      ~Client();

      void start();

      void onKeyUp(const SDL_KeyboardEvent& event) override;
      void onKeyDown(const SDL_KeyboardEvent& event) override;
    private:
      void tickSocket();
    private:
      _TCPsocket* mSocket;
  };
}

#endif

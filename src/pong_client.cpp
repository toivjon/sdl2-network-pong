#include "pong_client.h"

#include <SDL/SDL_net.h>

#include <cstdio>
#include <cstdlib>

using namespace pong;

Client::Client(const std::string& host) : Application(1)
{
  // resolve the host address of the given host.
  IPaddress ip;
  if (SDLNet_ResolveHost(&ip, host.c_str(), 6666) == -1) {
    printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
    exit(-1);
  }

  // open a new TCP socket to accept incoming connections.
  if ((mSocket = SDLNet_TCP_Open(&ip)) == NULL) {
    printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
    exit(-1);
  }

  // add the socket into the socket set.
  if (SDLNet_TCP_AddSocket(mSocketSet, mSocket) == -1) {
    printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
    exit(-1);
  }
}

Client::~Client()
{
  SDLNet_TCP_DelSocket(mSocketSet, mSocket);
  SDLNet_TCP_Close(mSocket);
}

void Client::start()
{
  setRunning(true);
  while (isRunning()) {
    tickSDL();
    tickSocket();
    render();
  }
}

void Client::tickSocket()
{
    auto socketSetState = SDLNet_CheckSockets(mSocketSet, 0);
    if (socketSetState == -1) {
      printf("SDLNet_CheckSocket: %s\n", SDLNet_GetError());
      perror("SDLNet_CheckSockets");
    } else if (socketSetState > 0) {
      // check whether we have received a message from the server.
      if (SDLNet_SocketReady(mSocket)) {
        char buffer[MAX_PACKAGE_SIZE];
        if (SDLNet_TCP_Recv(mSocket, buffer, MAX_PACKAGE_SIZE) <= 0) {
          printf("The connection was lost or it was denied or closed by the server.\n");
          setRunning(false);
        } else {
          printf("Received data from the server: %s\n", buffer);
        }
      }
    }
}

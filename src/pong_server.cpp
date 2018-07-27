#include "pong_server.h"

#include <SDL/SDL_net.h>

#include <cstdio>
#include <cstdlib>

using namespace pong;

Server::Server() : Application(2), mServerSocket(NULL), mClientSocket(NULL)
{
  // resolve the host address of the current machine.
  IPaddress ip;
  if (SDLNet_ResolveHost(&ip, NULL, 6666) == -1) {
    printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
    exit(-1);
  }

  // open a new TCP socket to accept incoming connections.
  if ((mServerSocket = SDLNet_TCP_Open(&ip)) == NULL) {
    printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
    exit(-1);
  }

  // add the server socket into the socket set.
  if (SDLNet_TCP_AddSocket(mSocketSet, mServerSocket) == -1) {
    printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
    exit(-1);
  }
}

Server::~Server()
{
  SDLNet_TCP_DelSocket(mSocketSet, mClientSocket);
  SDLNet_TCP_DelSocket(mSocketSet, mServerSocket);
  SDLNet_TCP_Close(mClientSocket);
  SDLNet_TCP_Close(mServerSocket);
}

void Server::start()
{
  setRunning(true);
  while (isRunning()) {
    tickSDL();
    tickSockets();
    render();
  }
}

void Server::tickSockets()
{
  // perform a non-blocking check whether sockets have activity.
  auto socketSetState = SDLNet_CheckSockets(mSocketSet, 0);
  if (socketSetState == -1) {
    printf("SDLNet_CheckSocket: %s\n", SDLNet_GetError());
    perror("SDLNet_CheckSockets");
  } else if (socketSetState > 0) {
    // check whether a new incoming connection is waiting.
    if (SDLNet_SocketReady(mServerSocket)) {
      // accept the incoming connection.
      auto newClient = SDLNet_TCP_Accept(mServerSocket);
      if (newClient == NULL) {
        printf("SDLNet_TCP_Accept: %s\n", SDLNet_GetError());
        exit(-1);
      }

      // check whether we already have a client with us.
      if (mClientSocket != NULL) {
        printf("Forbid a new client from joining as we already have a client.\n");
        SDLNet_TCP_Close(newClient);
      } else {
        // add the new client as a part of the sockets set.
        mClientSocket = newClient;
        if (SDLNet_TCP_AddSocket(mSocketSet, mClientSocket) == -1) {
          printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
          exit(-1);
        }
        printf("A client joined the server.\n");
      }
    }

    // check whether we received a message from the client.
    if (SDLNet_SocketReady(mClientSocket)) {
      char buffer[MAX_PACKAGE_SIZE];
      if (SDLNet_TCP_Recv(mClientSocket, buffer, MAX_PACKAGE_SIZE) <= 0) {
        printf("Client exited from the game.");
        SDLNet_TCP_DelSocket(mSocketSet, mClientSocket);
        SDLNet_TCP_Close(mClientSocket);
        mClientSocket = NULL;
        setRunning(false);
      }
      // TODO handle buffer contents...
    }
  }
}

#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// game resolution width in pixels.
#define RESOLUTION_WIDTH 800
// game resolution height in pixels.
#define RESOLUTION_HEIGHT 600

// the network port used by the application.
#define NETWORK_PORT 6666
// the TCP message buffer size.
#define NETWORK_TCP_BUFFER_SIZE 512
// the interval to send ping requests.
#define NETWORK_PING_INTERVAL 1000

// available network node modes.
enum Mode { CLIENT, SERVER };
// available application states.
enum State { RUNNING, STOPPED };

// ============================================================================

static void ping_send_response(int pingTime);
static int get_ticks_without_offset();
static int get_ticks();

// ============================================================================

// the network mode (client/server).
static int sMode = SERVER;
// the network host (NULL for server).
static char* sHost = NULL;

// the main window of the application.
static SDL_Window* sWindow = NULL;
// the renderer for the main window.
static SDL_Renderer* sRenderer = NULL;

// the socket used in the TCP communication.
static TCPsocket sTCPsocket = NULL;
// the message buffer for outgoing TCP stream data.
static char sTCPSend[NETWORK_TCP_BUFFER_SIZE];
// the message buffer for incoming TCP stream data.
static char sTCPRecv[NETWORK_TCP_BUFFER_SIZE];

// the TCP message stream buffer to handle network messages.
static char sStreamBuffer[NETWORK_TCP_BUFFER_SIZE];
// the TCP message stream cursor to follow stream content.
static int sStreamCursor = 0;

// the socket set used to listen for socket activities.
static SDLNet_SocketSet sSocketSet = NULL;

// the state of the application.
static int sState = RUNNING;
// the offset used to synchronize clocks among nodes.
static int sTickOffset = 0;
// the definition when to send next ping request.
static int sNextPingTicks = 0;

// ============================================================================

static void parse_arguments(int argc, char* argv[])
{
  // parse definitions from the provided command line arguments.
  printf("Parsing [%d] argument(s)...\n", (argc - 1));
  sMode = (argc > 1 ? CLIENT : SERVER);
  sHost = (argc <= 1 ? NULL : argv[1]);

  // inform about the successfully parse values.
  printf("Parsed following arguments from the command line:\n");
  printf("\tmode: %s\n", (sMode == CLIENT ? "client" : "server"));
  printf("\thost: %s\n", (sHost == NULL ? "" : sHost));
}

// ============================================================================
// destroy and release the application window.
static void destroy_window()
{
  SDL_DestroyWindow(sWindow);
}

// ============================================================================
// destroy and release the application window renderer.
static void destroy_renderer()
{
  SDL_DestroyRenderer(sRenderer);
}

// ============================================================================
// close and destroy the application TCP socket.
static void close_tcp_socket()
{
  SDLNet_TCP_Close(sTCPsocket);
}

// ============================================================================
// close and destroy the application's socket set.
static void close_socket_set()
{
  SDLNet_FreeSocketSet(sSocketSet);
}

// ============================================================================
// the main function to initialize the core application structure.
static void initialize(int argc, char* argv[])
{
  parse_arguments(argc, argv);

  // initialize the core SDL framework.
  if (SDL_Init(SDL_INIT_VIDEO) != 0) {
    printf("SDL_Init: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(SDL_Quit);

  // initialize the network SDL framework.
  if (SDLNet_Init() != 0) {
    printf("SDLNet_Init: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(SDLNet_Quit);

  // create the main window for the application.
  sWindow = SDL_CreateWindow(
    "Pong",
    SDL_WINDOWPOS_CENTERED,
    SDL_WINDOWPOS_CENTERED,
    RESOLUTION_WIDTH,
    RESOLUTION_HEIGHT,
    SDL_WINDOW_SHOWN);
  if (sWindow == NULL) {
    printf("SDL_CreateWindow: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(destroy_window);

  // create the main renderer for the application window.
  sRenderer = SDL_CreateRenderer(
    sWindow,
    -1,
    SDL_RENDERER_ACCELERATED);
  if (sRenderer == NULL) {
    printf("SDL_CreateRenderer: %s\n", SDL_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(destroy_renderer);
}

// ============================================================================
// a blocking call to start a TCP communication with a remote node.
static void tcp_start()
{
  // resolve the target host address.
  IPaddress ip;
  if (SDLNet_ResolveHost(&ip, sHost, NETWORK_PORT) != 0) {
    printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }

  // open a new TCP socket to target host for a network communication.
  printf("Opening a TCP socket for network communication...\n");
  sTCPsocket = SDLNet_TCP_Open(&ip);
  if (sTCPsocket == NULL) {
    printf("SDLNet_TCP_Open: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
  printf("Successfully opened a new TCP socket.\n");
  atexit(close_tcp_socket);

  // allocate a socket set to enable socket activity listening.
  sSocketSet = SDLNet_AllocSocketSet(1);
  if (sSocketSet == NULL) {
    printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(close_socket_set);

  // add the opened TCP socket into the socket set.
  if (SDLNet_TCP_AddSocket(sSocketSet, sTCPsocket) == -1) {
    printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }

  // make the server to wait until a client joins the game.
  if (sMode == SERVER) {
    printf("Waiting for a client to join the game...\n");
    if (SDLNet_CheckSockets(sSocketSet, -1) == -1) {
      printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
      perror("SDLNEt_CheckSockets");
      exit(EXIT_FAILURE);
    }

    // accept the incoming connection as a client.
    TCPsocket client = SDLNet_TCP_Accept(sTCPsocket);
    if (client == NULL) {
      printf("SDLNet_TCP_Accept: %s\n", SDLNet_GetError());
      exit(EXIT_FAILURE);
    }

    // remove and close the server socket from listening for new connections.
    if (SDLNet_TCP_DelSocket(sSocketSet, sTCPsocket) > 1) {
      printf("SDLNet_TCP_DelSocket: %s\n", SDLNet_GetError());
      exit(EXIT_FAILURE);
    }
    close_tcp_socket();

    // add the new client as a part of the socket set.
    sTCPsocket = client;
    if (SDLNet_TCP_AddSocket(sSocketSet, sTCPsocket) == -1) {
      printf("SDLNet_TCP_AddSocket: %s\n", SDLNet_GetError());
      exit(EXIT_FAILURE);
    }
    printf("A client successfully joined the game.\n");
  }
}

// ============================================================================
// send the given message to the remote node by using the TCP socket.
static void tcp_send(const char* msg)
{
  SDL_snprintf(sTCPSend, NETWORK_TCP_BUFFER_SIZE, "%s|", msg);
  int size = SDL_strlen(sTCPSend);
  if (SDLNet_TCP_Send(sTCPsocket, sTCPSend, size) != size) {
    printf("SDLNet_TCP_Send: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
}

// ============================================================================
// receive data from the remote node by using the TCP socket.
static void tcp_receive()
{
  // get the incoming stream data from the TCP socket.
  int bytes = SDLNet_TCP_Recv(sTCPsocket, sTCPRecv, NETWORK_TCP_BUFFER_SIZE);
  if (bytes <= 0) {
    printf("The connection to the remote node was lost.\n");
    exit(EXIT_FAILURE);
  }

  // process the received data stream.
  for (int i = 0; i < bytes; i++) {
    if (sTCPRecv[i] == '|') {
      if (sStreamCursor > 0) {
        char* token = strtok(sStreamBuffer, ":");
        if (strncmp(token, "ping", 4) == 0) {
          // get the ping time from the request.
          token = strtok(NULL, ":");
          int t0 = atoi(token);

          // send a pong response back to requester.
          ping_send_response(t0);
        } else if (strncmp(token, "pong", 4) == 0) {
          // get ping and pong times.
          token = strtok(NULL, ":");
          int t0 = atoi(token);
          token = strtok(NULL, ":");
          int t1 = atoi(token);

          // calculate latency and delta to adjust clock offset.
          int t2 = get_ticks();
          int rtt = (t2 - t0);
          int lag = (rtt / 2);
          if (sMode == SERVER) {
            printf("rtt:%d lag:%d\n", rtt, lag);
          } else {
            sTickOffset = ((t1 - t0) + (t1 - t2)) / 2;
            printf("rtt:%d lag:%d offset:%d\n", rtt, lag, sTickOffset);
          }
        }
        memset(sStreamBuffer, 0, NETWORK_TCP_BUFFER_SIZE);
        sStreamCursor = 0;
      }
    } else {
      sStreamBuffer[sStreamCursor] = sTCPRecv[i];
      sStreamCursor++;
    }
  }
}

// ============================================================================
// render and present all game objects on the screen.
static void render()
{
  // clear the backbuffer with the black color.
  SDL_SetRenderDrawColor(sRenderer, 0x00, 0x00, 0x00, 0x00);
  SDL_RenderClear(sRenderer);

  // TODO draw objects here...

  // swap backbuffer to front and vice versa.
  SDL_RenderPresent(sRenderer);
}

// ============================================================================
// get the current tick time without any offsets.
static int get_ticks_without_offset()
{
  return SDL_GetTicks();
}

// ============================================================================
// get the current tick time along with any time offset.
static int get_ticks()
{
  return SDL_GetTicks() + sTickOffset;
}

// ============================================================================
// send a ping request message to the remote node.
static void ping_send_request()
{
  char buffer[NETWORK_TCP_BUFFER_SIZE];
  SDL_snprintf(buffer, NETWORK_TCP_BUFFER_SIZE, "ping:%d", get_ticks());
  tcp_send(buffer);
}

// ============================================================================
// send a ping response message (pong) to the remote node.
static void ping_send_response(int ping)
{
  char buffer[NETWORK_TCP_BUFFER_SIZE];
  snprintf(buffer, NETWORK_TCP_BUFFER_SIZE, "pong:%d:%d", ping, get_ticks());
  tcp_send(buffer);
}

// ============================================================================

static void run()
{
  tcp_start();
  ping_send_request();
  SDL_Event event;
  while (sState == RUNNING) {
    // retrieve and handle core SDL events
    while(SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          sState = STOPPED;
          break;
      }
    }

    // peek to sockets and process the incoming data.
    int socketState = SDLNet_CheckSockets(sSocketSet, 0);
    if (socketState == -1) {
      printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
      perror("SDLNet_CheckSockets");
      break;
    } else if (socketState > 0) {
      tcp_receive();
    }

    // send ping request with the predefined interval.
    if (sNextPingTicks <= get_ticks_without_offset()) {
      ping_send_request();
      sNextPingTicks = get_ticks_without_offset() + NETWORK_PING_INTERVAL;
    }

    render();
  }
}

// ============================================================================

int main(int argc, char* argv[])
{
  initialize(argc, argv);
  run();
  return EXIT_SUCCESS;
}

#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>
#include <string.h>
#include <time.h>

// game resolution width in pixels.
#define RESOLUTION_WIDTH 800
// game resolution height in pixels.
#define RESOLUTION_HEIGHT 600

// game resolution width divided by two.
#define RESOLUTION_HALF_WIDTH (RESOLUTION_WIDTH / 2)
// game resolution height divided by two.
#define RESOLUTION_HALF_HEIGHT (RESOLUTION_HEIGHT / 2)

// the network port used by the application.
#define NETWORK_PORT 6666
// the network message buffer size.
#define NETWORK_BUFFER_SIZE 512
// the interval to send ping requests.
#define NETWORK_PING_INTERVAL 1000

// the size of a single graphical block in the scene.
#define BOX (RESOLUTION_HEIGHT / 30)
// the size of the single graphical block divided by two.
#define BOX_HALF (BOX / 2)

// the interval which is used to tick game logics.
#define TIMESTEP 17
// the maximum size of the state cache.
#define STATE_CACHE_SIZE 10
// the amount of milliseconds to wait before each ball launch.
#define COUNTDOWN_MS 2000
// the amount of milliseconds to wait before ending the game.
#define END_COUNTDOWN_MS 2000
// the target score limit.
#define SCORE_LIMIT 10

// the height for both paddles at the sides of the scene.
#define PADDLE_HEIGHT (RESOLUTION_HEIGHT / 6)
// the height for both paddles divided by two.
#define PADDLE_HALF_HEIGHT (PADDLE_HEIGHT / 2)
// the width for both paddles at the sides of the scene.
#define PADDLE_WIDTH BOX
// the amount of offset between paddle and the side of the scene.
#define PADDLE_EDGE_OFFSET (RESOLUTION_HEIGHT / 20)
// the movement velocity for the paddles.
#define PADDLE_VELOCITY (RESOLUTION_WIDTH / 100)

// the width of the ball.
#define BALL_WIDTH BOX
// the height of the ball.
#define BALL_HEIGHT BOX
// the initial velocity for the ball.
#define BALL_INITIAL_VELOCITY (RESOLUTION_HEIGHT / 300)
// the amount of velocity to increase on each ball-paddle hit.
#define BALL_VELOCITY_INCREMENT 1

// the width for the score indicator numbers.
#define SCORE_WIDTH (RESOLUTION_WIDTH / 10)
// the height for the score indicator numbers.
#define SCORE_HEIGHT (RESOLUTION_HEIGHT / 6)
// the half of the score indicator width.
#define SCORE_HALF_WIDTH (SCORE_WIDTH / 2)
// the half of the score indicator height.
#define SCORE_HALF_HEIGHT (SCORE_HEIGHT / 2)

// the line thickness for score indicator numbers.
#define SCORE_THICKNESS (SCORE_HEIGHT / 5)
// the half of the score indicator line thickness.
#define SCORE_HALF_THICKNESS (SCORE_THICKNESS / 2)

// the x-coordinate of the up-left position of the left score number.
#define SCORE_LEFT_X ((RESOLUTION_WIDTH / 2) - (2 * SCORE_WIDTH))
// the x-coordinate of the up-left position of the right score number.
#define SCORE_RIGHT_X ((RESOLUTION_WIDTH / 2) + SCORE_WIDTH)
// the y-coordinate of the up-left position of the left score number.
#define SCORE_Y (RESOLUTION_HEIGHT / 10)

// available network node modes.
enum Mode { CLIENT, SERVER };
// available application states.
enum State { RUNNING, STOPPED };
// available dynamic object movement directions.
enum Direction { UP = -1, DOWN = 1, LEFT = -1, RIGHT = 1, NONE = 0 };
// available network transport modes.
enum Transport { TCP, UDP };

// ============================================================================

// a function pointer type for sending messages over the network.
typedef void (*net_send_func)(const char*);
// a function pointer type for receiving messages from the network.
typedef void (*net_receive_func)();
// a function pointer type for the network system initialization.
typedef void (*net_start_func)();

// ============================================================================

typedef struct {
  // the timestamp of the state.
  int time;
  // the rect of the state.
  SDL_Rect rect;
} State;

typedef struct {
  // the array of dynamic object states.
  State states[STATE_CACHE_SIZE];
  // an index to the most recent state.
  int most_recent_state_index;
  // a definition whether the current node owns the object.
  int owned;
  // the movement speed.
  int velocity;
  // the movement direction in x-axis.
  int direction_x;
  // the movement direction in y-axis.
  int direction_y;
} DynamicObject;

// ============================================================================

// boundaries of the non-moving wall at the top of the scene.
const SDL_Rect TOP_WALL = { 0, 0, RESOLUTION_WIDTH, BOX };
// boundaries of the non-moving wall at the bottom of the scene.
const SDL_Rect BOTTOM_WALL = { 0, RESOLUTION_HEIGHT - BOX, RESOLUTION_WIDTH, BOX };
// the boundaries of the invisible left goal.
const SDL_Rect LEFT_GOAL = { -1000, 0, (1000 - BOX), RESOLUTION_HEIGHT };
// the boundaries of the invisible right goal.
const SDL_Rect RIGHT_GOAL = { RESOLUTION_WIDTH + BOX, 0, 1000, RESOLUTION_HEIGHT };

// boundaries of the center line containing 15 boxes.
const SDL_Rect CENTER_LINE[15] = {
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (0 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (1 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (2 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (3 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (4 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (5 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (6 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (7 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (8 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (9 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (10 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (11 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (12 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (13 * 1.93f * BOX), BOX, BOX},
  { RESOLUTION_HALF_WIDTH - BOX_HALF, BOX + (14 * 1.93f * BOX), BOX, BOX}
};

// the starting position of the left paddle.
const SDL_Rect LEFT_PADDLE_START = {
  PADDLE_EDGE_OFFSET,
  RESOLUTION_HALF_HEIGHT - PADDLE_HALF_HEIGHT,
  PADDLE_WIDTH,
  PADDLE_HEIGHT
};
// the starting position of the right paddle.
const SDL_Rect RIGHT_PADDLE_START = {
  RESOLUTION_WIDTH - PADDLE_EDGE_OFFSET - BOX,
  RESOLUTION_HALF_HEIGHT - PADDLE_HALF_HEIGHT,
  PADDLE_WIDTH,
  PADDLE_HEIGHT
};
// the starting position for the game ball.
const SDL_Rect BALL_START = {
  RESOLUTION_HALF_WIDTH - BOX_HALF,
  RESOLUTION_HALF_HEIGHT - BOX_HALF,
  BALL_WIDTH,
  BALL_HEIGHT
};

// horizontal line rectangles used with left score indicator.
const SDL_Rect SCORE_LEFT_PARTS[8] = {
  { SCORE_LEFT_X, SCORE_Y, SCORE_WIDTH, SCORE_THICKNESS },
  { SCORE_LEFT_X, SCORE_Y + SCORE_HALF_HEIGHT - SCORE_HALF_THICKNESS, SCORE_WIDTH, SCORE_THICKNESS},
  { SCORE_LEFT_X, SCORE_Y + SCORE_HEIGHT - SCORE_THICKNESS, SCORE_WIDTH, SCORE_THICKNESS },
  { SCORE_LEFT_X, SCORE_Y, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_LEFT_X, SCORE_Y + SCORE_HALF_HEIGHT, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_LEFT_X + SCORE_WIDTH - SCORE_THICKNESS, SCORE_Y, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_LEFT_X + SCORE_WIDTH - SCORE_THICKNESS, SCORE_Y + SCORE_HALF_HEIGHT, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_LEFT_X + SCORE_HALF_WIDTH - SCORE_THICKNESS, SCORE_Y, SCORE_THICKNESS, SCORE_HEIGHT }
};

// horizontal line rectangles used with right score indicator.
const SDL_Rect SCORE_RIGHT_PARTS[8] = {
  { SCORE_RIGHT_X, SCORE_Y, SCORE_WIDTH, SCORE_THICKNESS },
  { SCORE_RIGHT_X, SCORE_Y + SCORE_HALF_HEIGHT - SCORE_HALF_THICKNESS, SCORE_WIDTH, SCORE_THICKNESS},
  { SCORE_RIGHT_X, SCORE_Y + SCORE_HEIGHT - SCORE_THICKNESS, SCORE_WIDTH, SCORE_THICKNESS },
  { SCORE_RIGHT_X, SCORE_Y, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_RIGHT_X, SCORE_Y + SCORE_HALF_HEIGHT, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_RIGHT_X + SCORE_WIDTH - SCORE_THICKNESS, SCORE_Y, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_RIGHT_X + SCORE_WIDTH - SCORE_THICKNESS, SCORE_Y + SCORE_HALF_HEIGHT, SCORE_THICKNESS, SCORE_HALF_HEIGHT },
  { SCORE_RIGHT_X + SCORE_HALF_WIDTH - SCORE_THICKNESS, SCORE_Y, SCORE_THICKNESS, SCORE_HEIGHT }
};

// ============================================================================

static void ping_send_response(int pingTime);
static int get_ticks_without_offset();
static int get_ticks();
static int random_vertical_direction();
static int random_horizontal_direction();
static void reset_client(int time);
static void reset_server(int time);
static void tcp_send(const char* msg);
static void udp_send(const char* msg);
static void tcp_receive();
static void udp_receive();
static void tcp_start();
static void udp_start();

// ============================================================================

// the network mode (client/server).
static int sMode = SERVER;
// the network host (NULL for server).
static char* sHost = NULL;
// the network transport mode (TCP/UDP).
static int sTransport = TCP;

// the main window of the application.
static SDL_Window* sWindow = NULL;
// the renderer for the main window.
static SDL_Renderer* sRenderer = NULL;

// the socket used in the TCP communication.
static TCPsocket sTCPsocket = NULL;
// the message buffer for outgoing TCP stream data.
static char sTCPSend[NETWORK_BUFFER_SIZE];
// the message buffer for incoming TCP stream data.
static char sTCPRecv[NETWORK_BUFFER_SIZE];

// the TCP message stream buffer to handle network messages.
static char sStreamBuffer[NETWORK_BUFFER_SIZE];
// the TCP message stream cursor to follow stream content.
static int sStreamCursor = 0;

// the socket used in the UDP communication.
static UDPsocket sUDPsocket = NULL;
// the port and host used as the UDP communication target.
static IPaddress sUDPaddress;
// the outgoing package structure used to send UDP data.
static UDPpacket* sUDPSendPacket = NULL;

// the socket set used to listen for socket activities.
static SDLNet_SocketSet sSocketSet = NULL;

// the state of the application.
static int sState = RUNNING;
// the time (with offset) of the previous tick.
static int sPreviousTick = 0;
// the offset used to synchronize clocks among nodes.
static int sTickOffset = 0;
// the definition when to send next ping request.
static int sNextPingTicks = 0;
// the remote lag used to compensate latency.
static int sRemoteLag = 0;
// the countdown time used to detect when ball should be launched.
static int sCountdown = 0;
// the countdown time when the game ends and exits.
static int sEndCountdown = INT_MAX;

// the server's paddle shown at the left side of the scene.
static DynamicObject sLeftPaddle;
// the client's paddle shown at the right side of the scene.
static DynamicObject sRightPaddle;
// the ball moving across the scene.
static DynamicObject sBall;

// the points of the left player.
static int sLeftPoints = 0;
// the points of the right player.
static int sRightPoints = 0;

// a function pointer to a function to send data to remote node.
static net_send_func net_send = &tcp_send;
// a function pointer to a function to receive data from a remote node.
static net_receive_func net_receive = &tcp_receive;
// a function pointer to a function to initialize the network system.
static net_start_func net_start = &tcp_start;

// ============================================================================

static void parse_arguments(int argc, char* argv[])
{
  // parse definitions from the provided command line arguments.
  printf("Parsing [%d] argument(s)...\n", (argc - 1));
  sMode = (argc > 2 ? CLIENT : SERVER);
  sTransport = (argc <= 1 ? TCP : strncmp("tcp", argv[1], 3) == 0 ? TCP : UDP);
  sHost = (argc <= 2 ? NULL : argv[2]);

  // inform about the successfully parse values.
  printf("Parsed following arguments from the command line:\n");
  printf("\tmode: %s\n", (sMode == CLIENT ? "client" : "server"));
  printf("\thost: %s\n", (sHost == NULL ? "" : sHost));
  printf("\ttype: %s\n", (sTransport == TCP ? "TCP" : "UDP"));
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
// close and destroy the application UDP socket.
static void close_udp_socket()
{
  SDLNet_UDP_Close(sUDPsocket);
}

// ============================================================================
// close and destroy the application UDP send packet.
static void close_udp_send_packet()
{
  SDLNet_FreePacket(sUDPSendPacket);
}

// ============================================================================
// close and destroy the application's socket set.
static void close_socket_set()
{
  SDLNet_FreeSocketSet(sSocketSet);
}

// ============================================================================
// give a point to the target player.
static void give_point(int player)
{
  SDL_assert(player >= 0);
  SDL_assert(player <= 1);

  // increment points of the target player.
  printf("A point for %s player!\n", (player == 0 ? "left" : "right"));
  int endGame = 0;
  if (player == 0) {
    sLeftPoints++;
    endGame = (sLeftPoints >= SCORE_LIMIT ? 1 : 0);
  } else {
    sRightPoints++;
    endGame = (sRightPoints >= SCORE_LIMIT ? 1 : 0);
  }

  // check whether we should end the game.
  if (endGame == 1) {
    net_send("end");
  }
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

  // seed the random generator.
  srand(time(NULL));

  // initialize the paddle show at the left side of the scene.
  sLeftPaddle.owned = (sMode == SERVER ? 1 : 0);
  sLeftPaddle.most_recent_state_index = 0;
  sLeftPaddle.direction_x = NONE;
  sLeftPaddle.direction_y = NONE;
  sLeftPaddle.velocity = PADDLE_VELOCITY;
  for (int i = 0; i < STATE_CACHE_SIZE; i++) {
    sLeftPaddle.states[i].time = 0;
    sLeftPaddle.states[i].rect = LEFT_PADDLE_START;
  }

  // initialize the paddle show at the right side of the scene.
  sRightPaddle.owned = (sMode == CLIENT ? 1 : 0);
  sRightPaddle.most_recent_state_index = 0;
  sRightPaddle.direction_x = NONE;
  sRightPaddle.direction_y = NONE;
  sRightPaddle.velocity = PADDLE_VELOCITY;
  for (int i = 0; i < STATE_CACHE_SIZE; i++) {
    sRightPaddle.states[i].time = 0;
    sRightPaddle.states[i].rect = RIGHT_PADDLE_START;
  }

  // initialize the ball to start from the middle of the scene.
  sBall.owned = 1;
  sBall.most_recent_state_index = 0;
  sBall.direction_x = NONE;
  sBall.direction_y = NONE;
  sBall.velocity = BALL_INITIAL_VELOCITY;
  for (int i = 0; i < STATE_CACHE_SIZE; i++) {
    sBall.states[i].time = 0;
    sBall.states[i].rect = BALL_START;
  }

  // initialize function pointers and buffers to transport type functions.
  switch (sTransport) {
    case TCP:
      net_send = &tcp_send;
      net_receive = &tcp_receive;
      net_start = &tcp_start;
      break;
    case UDP:
      net_send = &udp_send;
      net_receive = &udp_receive;
      net_start = &udp_start;
      break;
    default:
      printf("Unsupported transport %d!\n", sTransport);
      exit(EXIT_FAILURE);
      break;
  }
}

// ============================================================================
// Get a rect for the given time for the target object.
static SDL_Rect state_get(DynamicObject* object, int time)
{
  SDL_assert(object != NULL);

  // state checks are only required for non-owned objects.
  if (object->owned != 1) {
    // apply remote lag to keep non-owned objects in the past to compensate lag.
    time = time - sRemoteLag;

    int index = (object->most_recent_state_index + 1) % STATE_CACHE_SIZE;
    if (object->states[index].time > time) {
      return object->states[index].rect;
    } else {
      index = object->most_recent_state_index;
      if (object->states[index].time <= time) {
        return object->states[index].rect;
      } else {
        for (int i = 0; i < STATE_CACHE_SIZE; i++) {
          index = abs(object->most_recent_state_index - i) % STATE_CACHE_SIZE;
          if (object->states[index].time == time) {
            return object->states[index].rect;
          } else if (object->states[index].time > time) {
            // calculate previous index and a time scalar.
            int prevIndex = (index + 1) % STATE_CACHE_SIZE;
            float t = (object->states[index].time - time) /
                      (object->states[prevIndex].time - time);
            SDL_Rect* prevRect = &object->states[prevIndex].rect;

            // calculate a new rect by using a linear interpolation.
            SDL_Rect rect = object->states[index].rect;
            rect.x += (int)(t * (float)(prevRect->x - rect.x));
            rect.y += (int)(t * (float)(prevRect->y - rect.y));
            return rect;
          }
        }
      }
    }
  }
  return object->states[object->most_recent_state_index].rect;
}

// ============================================================================
// set the given rect as a state for the given object at the given time.
static void state_set(DynamicObject* object, const SDL_Rect* rect, int time)
{
  SDL_assert(object != NULL);
  SDL_assert(rect != NULL);

  // increment the most recent state index by one.
  int index = object->most_recent_state_index;
  index = (index + 1) % STATE_CACHE_SIZE;
  object->most_recent_state_index = index;

  // add the given state as the most recent state.
  object->states[object->most_recent_state_index].time = time;
  object->states[object->most_recent_state_index].rect = *rect;
}

// ============================================================================
// set all states to given rect after the given from time point.
static void state_clear(DynamicObject* object, const SDL_Rect* rect, int from)
{
  SDL_assert(object != NULL);
  SDL_assert(rect != NULL);
  SDL_assert(from > 0);

  // clear all states starting from the given time.
  for (int i = 0; i < STATE_CACHE_SIZE; i++) {
    if (object->states[i].time >= from) {
      object->states[i].time = from;
      object->states[i].rect = *rect;
    }
  }
}

// ============================================================================
// start a UDP communication with a remote node.
static void udp_start()
{
  SDL_assert(sTransport == UDP);
  SDL_assert(sUDPsocket == NULL);

  // open a socket to be used for UDP packet sending and receiving.
  sUDPsocket = SDLNet_UDP_Open(sMode == SERVER ? NETWORK_PORT : 0);
  if (sUDPsocket == NULL) {
    printf("SDLNet_UDP_Open: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
  printf("Successfully opened a new UDP socket.\n");
  atexit(close_udp_socket);

  // allocate a socket set to enable socket activity listening.
  sSocketSet = SDLNet_AllocSocketSet(1);
  if (sSocketSet == NULL) {
    printf("SDLNet_AllocSocketSet: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(close_socket_set);

  // add the opened TCP socket into the socket set.
  if (SDLNet_UDP_AddSocket(sSocketSet, sUDPsocket) == -1) {
    printf("SDLNet_UDP_AddSocket: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }

  // allocate memory for the UDP packet to be used with outgoing data.
  sUDPSendPacket = SDLNet_AllocPacket(NETWORK_BUFFER_SIZE);
  if (sUDPSendPacket == NULL) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
  atexit(close_udp_send_packet);

  // make the server to wait until a client joins the game.
  if (sMode == SERVER) {
    printf("Waiting for a client to join the game...\n");
    if (SDLNet_CheckSockets(sSocketSet, -1) == -1) {
      printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
      perror("SDLNEt_CheckSockets");
      exit(EXIT_FAILURE);
    }
    printf("Client connected?\n");
    net_receive();
  } else {
    // resolve the target host address.
    if (SDLNet_ResolveHost(&sUDPaddress, sHost, NETWORK_PORT) != 0) {
      printf("SDLNet_ResolveHost: %s\n", SDLNet_GetError());
      exit(EXIT_FAILURE);
    }

    // send the initial joining message to the server.
    printf("Sending a hello message to server...\n");
    net_send("hello");
    // TODO: wait and ensure that we get a response from the server.
  }
}

// ============================================================================
// send the given message to the remote node by using the UDP socket.
static void udp_send(const char* msg)
{
  SDL_assert(msg != NULL);
  SDL_assert(sTransport == UDP);

  // copy the target message into the packet.
  int size = SDL_strlen(msg);
  SDL_assert(size <= NETWORK_BUFFER_SIZE);
  memcpy(sUDPSendPacket->data, msg, size);
  sUDPSendPacket->len = size;
  sUDPSendPacket->address = sUDPaddress;

  // send the given packet to remote nodes.
  int sent = SDLNet_UDP_Send(sUDPsocket, -1, sUDPSendPacket);
  if (sent == 0) {
    printf("SDLNet_UDP_Send: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }
}

// ============================================================================
// receive data from the remote node by using the UDP socket.
static void udp_receive()
{
  SDL_assert(sTransport == UDP);

  // allocate memory for a new UDP packet.
  UDPpacket* packet = SDLNet_AllocPacket(NETWORK_BUFFER_SIZE);
  if (packet == NULL) {
    printf("SDLNet_AllocPacket: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  }

  int packets = SDLNet_UDP_Recv(sUDPsocket, packet);
  if (packets == -1) {
    printf("SDLNet_UDP_Rect: %s\n", SDLNet_GetError());
    exit(EXIT_FAILURE);
  } else if (packets == 1) {
    // ensure that we use the source address for outgoing messages.
    sUDPaddress = packet->address;

    // copy the UDP package contents into incoming buffer.
    char buffer[NETWORK_BUFFER_SIZE];
    memcpy(buffer, packet->data, packet->len);
    buffer[packet->len] = '\0';

    // process the message by using a tokenization.
    char* token = strtok(buffer, ":");
    if (strncmp(token, "quit", 4) == 0) {
      printf("Remote node has closed the connection: Closing application...\n");
      sState = STOPPED;
    } else if (strncmp(token, "ping", 4) == 0) {
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

      // calculate latency and delta to adjust clock offset and remote lag.
      int t2 = get_ticks();
      int rtt = (t2 - t0);
      int lag = (rtt / 2);
      sRemoteLag = lag + (50 - (lag % 50));
      if (sMode == SERVER) {
        printf("rtt:%d remoteLag:%d\n", rtt, sRemoteLag);
      } else {
        int cc = ((t1 - t0) + (t1 - t2)) / 2;
        sTickOffset += cc;
        printf("rtt:%d remoteLag:%d cc:%d co:%d\n", rtt, sRemoteLag, cc, sTickOffset);
      }
    } else if (strncmp(token, "left", 4) == 0) {
        // get time, x and y positions.
        token = strtok(NULL, ":");
        int t = atoi(token);
        token = strtok(NULL, ":");
        int x = atoi(token);
        token = strtok(NULL, ":");
        int y = atoi(token);

        // create a new state and assign it to states array.
        SDL_Rect rect = {x, y, PADDLE_WIDTH, PADDLE_HEIGHT };
        state_set(&sLeftPaddle, &rect, t);
    } else if (strncmp(token, "right", 5) == 0) {
      // get time, x and y positions.
      token = strtok(NULL, ":");
      int t = atoi(token);
      token = strtok(NULL, ":");
      int x = atoi(token);
      token = strtok(NULL, ":");
      int y = atoi(token);

      // create a new state and assign it to states array.
      SDL_Rect rect = {x, y, PADDLE_WIDTH, PADDLE_HEIGHT };
      state_set(&sRightPaddle, &rect, t);
    } else if (strncmp(token, "ball", 4) == 0) {
      // get time, x and y positions.
      token = strtok(NULL, ":");
      int t = atoi(token);
      token = strtok(NULL, ":");
      int x = atoi(token);
      token = strtok(NULL, ":");
      int y = atoi(token);
      token = strtok(NULL, ":");
      int dirX = atoi(token);
      token = strtok(NULL, ":");
      int dirY = atoi(token);
      token = strtok(NULL, ":");
      int velocity = atoi(token);

      // check if we need to correct the position and direction of the ball.
      SDL_Rect rect = {x, y, BALL_WIDTH, BALL_HEIGHT };
      SDL_Rect usedRect = state_get(&sBall, t);
      if (usedRect.x != rect.x || usedRect.y != rect.y
        || dirX != sBall.direction_x || dirY != sBall.direction_y
        || velocity != sBall.velocity) {
        state_clear(&sBall, &rect, t);
        state_set(&sBall, &rect, t);
        sBall.direction_x = dirX;
        sBall.direction_y = dirY;
        sBall.velocity = velocity;
      }
    } else if (strncmp(token, "reset", 5) == 0) {
      SDL_assert(sMode == CLIENT);

      // get time, countdown, ball directions and points.
      token = strtok(NULL, ":");
      int t = atoi(token);
      token = strtok(NULL, ":");
      sCountdown = atoi(token);
      token = strtok(NULL, ":");
      int x = atoi(token);
      token = strtok(NULL, ":");
      int y = atoi(token);
      token = strtok(NULL, ":");
      sLeftPoints = atoi(token);
      token = strtok(NULL, ":");
      sRightPoints = atoi(token);

      // assign ball directions.
      sBall.direction_x = x;
      sBall.direction_y = y;

      // perform a client reset.
      reset_client(t);
    } else if (strncmp(token, "goal", 4) == 0) {
      SDL_assert(sMode == SERVER);
      give_point(0);
      reset_server(get_ticks());
    } else if (strncmp(token, "end-ok", 6) == 0) {
      SDL_assert(sMode == SERVER);
      sEndCountdown = get_ticks_without_offset() + END_COUNTDOWN_MS;
    } else if (strncmp(token, "end", 3) == 0) {
      SDL_assert(sMode == CLIENT);
      sEndCountdown = get_ticks_without_offset() + END_COUNTDOWN_MS;
      net_send("end-ok");
    }
  } else {
    printf("There was %d packets in the UDP queue!\n", packets);
  }

  // release memory reserved for the packet.
  SDLNet_FreePacket(packet);
}

// ============================================================================
// a blocking call to start a TCP communication with a remote node.
static void tcp_start()
{
  SDL_assert(sTransport == TCP);

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
  SDL_snprintf(sTCPSend, NETWORK_BUFFER_SIZE, "%s|", msg);
  int size = SDL_strlen(sTCPSend);
  SDL_assert(size <= NETWORK_BUFFER_SIZE);
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
  int bytes = SDLNet_TCP_Recv(sTCPsocket, sTCPRecv, NETWORK_BUFFER_SIZE);
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

          // calculate latency and delta to adjust clock offset and remote lag.
          int t2 = get_ticks();
          int rtt = (t2 - t0);
          int lag = (rtt / 2);
          sRemoteLag = lag + (50 - (lag % 50));
          if (sMode == SERVER) {
            printf("rtt:%d remoteLag:%d\n", rtt, sRemoteLag);
          } else {
            int cc = ((t1 - t0) + (t1 - t2)) / 2;
            sTickOffset += cc;
            printf("rtt:%d remoteLag:%d cc:%d co:%d\n", rtt, sRemoteLag, cc, sTickOffset);
          }
        } else if (strncmp(token, "left", 4) == 0) {
          // get time, x and y positions.
          token = strtok(NULL, ":");
          int t = atoi(token);
          token = strtok(NULL, ":");
          int x = atoi(token);
          token = strtok(NULL, ":");
          int y = atoi(token);

          // create a new state and assign it to states array.
          SDL_Rect rect = {x, y, PADDLE_WIDTH, PADDLE_HEIGHT };
          state_set(&sLeftPaddle, &rect, t);
        } else if (strncmp(token, "right", 5) == 0) {
          // get time, x and y positions.
          token = strtok(NULL, ":");
          int t = atoi(token);
          token = strtok(NULL, ":");
          int x = atoi(token);
          token = strtok(NULL, ":");
          int y = atoi(token);

          // create a new state and assign it to states array.
          SDL_Rect rect = {x, y, PADDLE_WIDTH, PADDLE_HEIGHT };
          state_set(&sRightPaddle, &rect, t);
        } else if (strncmp(token, "ball", 4) == 0) {
          // get time, x and y positions.
          token = strtok(NULL, ":");
          int t = atoi(token);
          token = strtok(NULL, ":");
          int x = atoi(token);
          token = strtok(NULL, ":");
          int y = atoi(token);
          token = strtok(NULL, ":");
          int dirX = atoi(token);
          token = strtok(NULL, ":");
          int dirY = atoi(token);
          token = strtok(NULL, ":");
          int velocity = atoi(token);

          // check if we need to correct the position and direction of the ball.
          SDL_Rect rect = {x, y, BALL_WIDTH, BALL_HEIGHT };
          SDL_Rect usedRect = state_get(&sBall, t);
          if (usedRect.x != rect.x || usedRect.y != rect.y
            || dirX != sBall.direction_x || dirY != sBall.direction_y
            || velocity != sBall.velocity) {
            state_clear(&sBall, &rect, t);
            state_set(&sBall, &rect, t);
            sBall.direction_x = dirX;
            sBall.direction_y = dirY;
            sBall.velocity = velocity;
          }
        } else if (strncmp(token, "reset", 5) == 0) {
          SDL_assert(sMode == CLIENT);

          // get time, countdown, ball directions and points.
          token = strtok(NULL, ":");
          int t = atoi(token);
          token = strtok(NULL, ":");
          sCountdown = atoi(token);
          token = strtok(NULL, ":");
          int x = atoi(token);
          token = strtok(NULL, ":");
          int y = atoi(token);
          token = strtok(NULL, ":");
          sLeftPoints = atoi(token);
          token = strtok(NULL, ":");
          sRightPoints = atoi(token);

          // assign ball directions.
          sBall.direction_x = x;
          sBall.direction_y = y;

          // perform a client reset.
          reset_client(t);
        } else if (strncmp(token, "goal", 4) == 0) {
          SDL_assert(sMode == SERVER);
          give_point(0);
          reset_server(get_ticks());
        } else if (strncmp(token, "end-ok", 6) == 0) {
          SDL_assert(sMode == SERVER);
          sEndCountdown = get_ticks_without_offset() + END_COUNTDOWN_MS;
        } else if (strncmp(token, "end", 3) == 0) {
          SDL_assert(sMode == CLIENT);
          sEndCountdown = get_ticks_without_offset() + END_COUNTDOWN_MS;
          tcp_send("end-ok");
        }
        memset(sStreamBuffer, 0, NETWORK_BUFFER_SIZE);
        sStreamCursor = 0;
      }
    } else {
      sStreamBuffer[sStreamCursor] = sTCPRecv[i];
      sStreamCursor++;
    }
  }
}

// ============================================================================
// render a point on the screen.
static void render_point(const SDL_Rect pointParts[8], int points)
{
  SDL_assert(pointParts != NULL);
  SDL_assert(points >= 0);
  switch (points) {
    case 0:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[3]);
      SDL_RenderFillRect(sRenderer, &pointParts[4]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    case 1:
      SDL_RenderFillRect(sRenderer, &pointParts[7]);
      break;
    case 2:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[4]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      break;
    case 3:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    case 4:
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[3]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    case 5:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[3]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    case 6:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[3]);
      SDL_RenderFillRect(sRenderer, &pointParts[4]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    case 7:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    case 8:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[3]);
      SDL_RenderFillRect(sRenderer, &pointParts[4]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
    default:
      SDL_RenderFillRect(sRenderer, &pointParts[0]);
      SDL_RenderFillRect(sRenderer, &pointParts[1]);
      SDL_RenderFillRect(sRenderer, &pointParts[2]);
      SDL_RenderFillRect(sRenderer, &pointParts[3]);
      SDL_RenderFillRect(sRenderer, &pointParts[5]);
      SDL_RenderFillRect(sRenderer, &pointParts[6]);
      break;
  }
}

// ============================================================================
// render and present all game objects on the screen.
static void render(int time)
{
  // resolve the current position of each dynamic game object.
  SDL_Rect leftPaddle = state_get(&sLeftPaddle, time);
  SDL_Rect rightPaddle = state_get(&sRightPaddle, time);
  SDL_Rect ball = state_get(&sBall, time);

  // clear the backbuffer with the black color.
  SDL_SetRenderDrawColor(sRenderer, 0x00, 0x00, 0x00, 0x00);
  SDL_RenderClear(sRenderer);

  // render all game objects on the backbuffer.
  SDL_SetRenderDrawColor(sRenderer, 0xff, 0xff, 0xff, 0xff);
  SDL_RenderFillRect(sRenderer, &TOP_WALL);
  SDL_RenderFillRect(sRenderer, &BOTTOM_WALL);
  SDL_RenderFillRects(sRenderer, CENTER_LINE, 15);
  SDL_RenderFillRect(sRenderer, &leftPaddle);
  SDL_RenderFillRect(sRenderer, &rightPaddle);
  SDL_RenderFillRect(sRenderer, &ball);

  // render point indicators.
  render_point(SCORE_LEFT_PARTS, sLeftPoints);
  render_point(SCORE_RIGHT_PARTS, sRightPoints);

  // swap backbuffer to front and vice versa.
  SDL_RenderPresent(sRenderer);
}

// ============================================================================
// update all game objects in a node specific way.
static void update(int time)
{
  // resolve the current position of each dynamic game object.
  SDL_Rect left = state_get(&sLeftPaddle, sPreviousTick);
  SDL_Rect right = state_get(&sRightPaddle, sPreviousTick);
  SDL_Rect ball = state_get(&sBall, sPreviousTick);

  // update the left paddle whether it's being owned and actually moving.
  if (sLeftPaddle.owned == 1 && sLeftPaddle.direction_y != NONE) {
    // move the paddle based on the movement direction.
    left.y += sLeftPaddle.velocity * sLeftPaddle.direction_y;

    // ensure that the top and bottom wall boundaries are honoured.
    if (SDL_HasIntersection(&left, &TOP_WALL)) {
      left.y = (TOP_WALL.y + TOP_WALL.h);
    } else if (SDL_HasIntersection(&left, &BOTTOM_WALL)) {
      left.y = (BOTTOM_WALL.y - PADDLE_HEIGHT);
    }

    // update the local state with the new position.
    state_set(&sLeftPaddle, &left, time);

    // send a state update about the movement to remote node.
    char buffer[NETWORK_BUFFER_SIZE];
    snprintf(buffer, NETWORK_BUFFER_SIZE, "left:%d:%d:%d", time, left.x, left.y);
    net_send(buffer);
  }

  // update the right paddle whether it's being owned and actually moving.
  if (sRightPaddle.owned == 1 && sRightPaddle.direction_y != NONE) {
    // move the paddle based on the movement direction.
    right.y += sRightPaddle.velocity * sRightPaddle.direction_y;

    // ensure that the top and bottom wall boundaries are honoured.
    if (SDL_HasIntersection(&right, &TOP_WALL)) {
      right.y = (TOP_WALL.y + TOP_WALL.h);
    } else if (SDL_HasIntersection(&right, &BOTTOM_WALL)) {
      right.y = (BOTTOM_WALL.y - PADDLE_HEIGHT);
    }

    // update the local state with the new position.
    state_set(&sRightPaddle, &right, time);

    // send a state update about the movement to remote node.
    char buffer[NETWORK_BUFFER_SIZE];
    snprintf(buffer, NETWORK_BUFFER_SIZE, "right:%d:%d:%d", time, right.x, right.y);
    net_send(buffer);
  }

  // update the movement of the ball.
  if (sBall.velocity != 0) {
    // move the ball based on the movement direction.
    ball.y += sBall.velocity * sBall.direction_y;
    ball.x += sBall.velocity * sBall.direction_x;

    // check whether the ball hits top or bottom walls.
    if (SDL_HasIntersection(&ball, &TOP_WALL)) {
      ball.y = (TOP_WALL.y + TOP_WALL.h);
      sBall.direction_y *= -1;
    } else if (SDL_HasIntersection(&ball, &BOTTOM_WALL)) {
      ball.y = (BOTTOM_WALL.y - ball.h);
      sBall.direction_y *= -1;
    }

    // check paddle collisions and send updates when required.
    if (SDL_HasIntersection(&left, &ball)) {
      ball.x = (left.x + left.w);
      sBall.direction_x *= -1;
      sBall.velocity += BALL_VELOCITY_INCREMENT;
      if (sLeftPaddle.owned == 1) {
        // send a state update about the movement to remote node.
        char buffer[NETWORK_BUFFER_SIZE];
        snprintf(buffer,
          NETWORK_BUFFER_SIZE,
          "ball:%d:%d:%d:%d:%d:%d",
          time,
          ball.x,
          ball.y,
          sBall.direction_x,
          sBall.direction_y,
          sBall.velocity);
        net_send(buffer);
      }
    } else if (SDL_HasIntersection(&right, &ball)) {
      ball.x = (right.x - ball.w);
      sBall.direction_x *= -1;
      sBall.velocity += BALL_VELOCITY_INCREMENT;
      if (sRightPaddle.owned == 1) {
        // send a state update about the movement to remote node.
        char buffer[NETWORK_BUFFER_SIZE];
        snprintf(buffer,
          NETWORK_BUFFER_SIZE,
          "ball:%d:%d:%d:%d:%d:%d",
          time,
          ball.x,
          ball.y,
          sBall.direction_x,
          sBall.direction_y,
          sBall.velocity);
        net_send(buffer);
      }
    }

    // check whether the ball hit a goal (decide on a owner side).
    if (sLeftPaddle.owned == 1) {
      if (SDL_HasIntersection(&ball, &LEFT_GOAL)) {
        give_point(1);
        reset_server(time);
        return;
      }
    } else {
      if (SDL_HasIntersection(&ball, &RIGHT_GOAL)) {
        net_send("goal");
        reset_client(time);
        return;
      }
    }

    // update the local state with the new position.
    state_set(&sBall, &ball, time);
  }
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
  char buffer[NETWORK_BUFFER_SIZE];
  SDL_snprintf(buffer, NETWORK_BUFFER_SIZE, "ping:%d", get_ticks());
  net_send(buffer);
}

// ============================================================================
// send a ping response message (pong) to the remote node.
static void ping_send_response(int ping)
{
  char buffer[NETWORK_BUFFER_SIZE];
  snprintf(buffer, NETWORK_BUFFER_SIZE, "pong:%d:%d", ping, get_ticks());
  net_send(buffer);
}

// ============================================================================
// get a random vertical direction (UP / DOWN)
static int random_vertical_direction()
{
  return (rand() % 2) == 0 ? UP : DOWN;
}

// ============================================================================
// get a random horizontal direction (LEFT / RIGHT).
static int random_horizontal_direction()
{
  return (rand() % 2) == 0 ? LEFT : RIGHT;
}

// ============================================================================
// reset the game (except points) at the client side.
static void reset_client(int time)
{
  SDL_assert(sMode == CLIENT);

  // clear all possible future states from the dynamic objects.
  state_clear(&sRightPaddle, &RIGHT_PADDLE_START, time);
  state_clear(&sLeftPaddle, &LEFT_PADDLE_START, time);
  state_clear(&sBall, &BALL_START, time);

  // reset dynamic objects back to their starting positions.
  state_set(&sRightPaddle, &RIGHT_PADDLE_START, time);
  state_set(&sLeftPaddle, &LEFT_PADDLE_START, time);
  state_set(&sBall, &BALL_START, time);

  // reset ball velocity back to initial velocity.
  sBall.velocity = BALL_INITIAL_VELOCITY;
}

// ============================================================================
// reset the game (except points) at the server side.
static void reset_server(int time)
{
  SDL_assert(sMode == SERVER);

  // clear all possible future states from the dynamic objects.
  state_clear(&sRightPaddle, &RIGHT_PADDLE_START, time);
  state_clear(&sLeftPaddle, &LEFT_PADDLE_START, time);
  state_clear(&sBall, &BALL_START, time);

  // reset dynamic objects back to their starting positions.
  state_set(&sRightPaddle, &RIGHT_PADDLE_START, time);
  state_set(&sLeftPaddle, &LEFT_PADDLE_START, time);
  state_set(&sBall, &BALL_START, time);

  // reset ball velocity back to initial velocity.
  sBall.velocity = BALL_INITIAL_VELOCITY;

  // assign a countdown to prevent ball from launching immediately.
  sCountdown = time + COUNTDOWN_MS;

  // randomize new horizontal- and vertical directions for the ball.
  sBall.direction_x = random_horizontal_direction();
  sBall.direction_y = random_vertical_direction();

  // send the reset command to client as well.
  char buffer[NETWORK_BUFFER_SIZE];
  snprintf(buffer,
    NETWORK_BUFFER_SIZE,
    "reset:%d:%d:%d:%d:%d:%d",
    time, sCountdown, sBall.direction_x, sBall.direction_y,
    sLeftPoints, sRightPoints);
  net_send(buffer);
}

// ============================================================================

static void run()
{
  net_start();
  ping_send_request();

  int deltaAccumulator = 0;
  int ticks = get_ticks_without_offset();
  int previousTicks = ticks;

  if (sMode == SERVER) {
    reset_server(ticks);
  }

  SDL_Event event;
  while (sState == RUNNING) {
    // get a ticks time and calculate delta.
    ticks = get_ticks_without_offset();
    int dt = (ticks - previousTicks);
    previousTicks = ticks;

    // retrieve and handle core SDL events
    while(SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          sState = STOPPED;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              if (sMode == SERVER) {
                sLeftPaddle.direction_y = UP;
              } else {
                sRightPaddle.direction_y = UP;
              }
              break;
            case SDLK_DOWN:
              if (sMode == SERVER) {
                sLeftPaddle.direction_y = DOWN;
              } else {
                sRightPaddle.direction_y = DOWN;
              }
              break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              if (sMode == SERVER) {
                if (sLeftPaddle.direction_y == UP) {
                  sLeftPaddle.direction_y = NONE;
                }
              } else {
                if (sRightPaddle.direction_y == UP) {
                  sRightPaddle.direction_y = NONE;
                }
              }
              break;
            case SDLK_DOWN:
              if (sMode == SERVER) {
                if (sLeftPaddle.direction_y == DOWN) {
                  sLeftPaddle.direction_y = NONE;
                }
              } else {
                if (sRightPaddle.direction_y == DOWN) {
                  sRightPaddle.direction_y = NONE;
                }
              }
              break;
          }
          break;
      }
    }

    // check whether it's time to end the game.
    if (sEndCountdown <= ticks) {
      sState = STOPPED;
      break;
    }

    // peek to sockets and process the incoming data.
    int socketState = SDLNet_CheckSockets(sSocketSet, 0);
    if (socketState == -1) {
      printf("SDLNet_CheckSockets: %s\n", SDLNet_GetError());
      perror("SDLNet_CheckSockets");
      break;
    } else if (socketState > 0) {
      net_receive();
    }

    // send ping request with the predefined interval.
    if (sNextPingTicks <= ticks) {
      ping_send_request();
      sNextPingTicks = get_ticks_without_offset() + NETWORK_PING_INTERVAL;
    }

    // update game logics with a fixed framerate.
    int time = get_ticks();
    deltaAccumulator += dt;
    if (deltaAccumulator >= TIMESTEP) {
      if (sCountdown <= time) {
        update(time);
      }
      deltaAccumulator -= TIMESTEP;
    }
    render(time);
    sPreviousTick = time;
  }
  if (sTransport == UDP) {
    net_send("quit");
  }
  printf("game ended with results %d - %d\n", sLeftPoints, sRightPoints);
}

// ============================================================================

int main(int argc, char* argv[])
{
  initialize(argc, argv);
  run();
  return EXIT_SUCCESS;
}

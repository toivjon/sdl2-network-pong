#define __USE_MINGW_ANSI_STDIO 0

#include <SDL/SDL.h>
#include <SDL/SDL_net.h>

#include <chrono>
#include <cstdio>
#include <random>
#include <sstream>
#include <string>
#include <vector>

using namespace std::chrono;

// ===============================
// = Constants and Configuration =
// ===============================

// game resolution width in pixels.
const auto RESOLUTION_WIDTH = 800;
// game resolution height in pixels.
const auto RESOLUTION_HEIGHT = 600;

const auto DIRECTION_UP    = -1;
const auto DIRECTION_DOWN  =  1;
const auto DIRECTION_LEFT  = -1;
const auto DIRECTION_RIGHT =  1;
const auto DIRECTION_NONE  =  0;

const auto MAX_PACKAGE_SIZE = 512;
const auto SERVER_INIT_TIMEOUT = 1000 * 60 * 5;
const auto NETWORK_SEND_INTERVAL = 10ll;

const auto BOX_WIDTH = (RESOLUTION_HEIGHT / 30);

const auto PADDLE_HEIGHT = (RESOLUTION_HEIGHT / 6);
const auto PADDLE_VELOCITY = (RESOLUTION_WIDTH / 100);
const auto PADDLE_EDGE_OFFSET = (RESOLUTION_HEIGHT / 20);

const auto BALL_INITIAL_VELOCITY = 2;
const auto BALL_MAX_VELOCITY = 8;
const auto BALL_COUNTDOWN = 1000;
const auto BALL_VELOCITY_INCREMENT = 1;

// ==================
// = Static Objects =
// ==================

const SDL_Rect TOP_WALL = { 0, 0, RESOLUTION_WIDTH, BOX_WIDTH };
const SDL_Rect BOTTOM_WALL = { 0, (RESOLUTION_HEIGHT - BOX_WIDTH), RESOLUTION_WIDTH, BOX_WIDTH };
const SDL_Rect LEFT_GOAL = { -1000, 0, (1000 - BOX_WIDTH), RESOLUTION_HEIGHT };
const SDL_Rect RIGHT_GOAL = { RESOLUTION_WIDTH + BOX_WIDTH, 0, 1000, RESOLUTION_HEIGHT };
const SDL_Rect CENTER_LINE[15] = {
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (0 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (1 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (2 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (3 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (4 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (5 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (6 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (7 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (8 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (9 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (10 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (11 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (12 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (13 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
  { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), static_cast<int>(BOX_WIDTH + (14 * 1.93f * BOX_WIDTH)), BOX_WIDTH, BOX_WIDTH },
};

// ===================
// = Dynamic Objects =
// ===================

// the starting position of the left paddle.
const SDL_Rect LEFT_PADDLE_START = {
  PADDLE_EDGE_OFFSET,
  ((RESOLUTION_HEIGHT / 2) - (PADDLE_HEIGHT / 2)),
  BOX_WIDTH,
  PADDLE_HEIGHT
};
// the starting position of the right paddle.
const SDL_Rect RIGHT_PADDLE_START = {
  (RESOLUTION_WIDTH - PADDLE_EDGE_OFFSET - BOX_WIDTH),
  ((RESOLUTION_HEIGHT / 2) - (PADDLE_HEIGHT / 2)),
  BOX_WIDTH,
  PADDLE_HEIGHT
};
// the starting position of the ball.
const SDL_Rect BALL_START = {
  ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)),
  ((RESOLUTION_HEIGHT / 2) - (BOX_WIDTH / 2)),
  BOX_WIDTH,
  BOX_WIDTH
};

// =======================
// = RANDOM DISTRIBUTION =
// =======================

// A shortcut to generate a random direction (-1 or 1).
inline int randomDirection() {
  static std::default_random_engine generator;
  static std::uniform_int_distribution<int> distribution(0, 1);
  return (distribution(generator) == 0 ? -1 : 1);
}

inline std::string serialize(const std::string& id, const SDL_Rect& rect) {
  std::string data;
  data.append(id);
  data.append(" ");
  data.append(std::to_string(rect.x));
  data.append(" ");
  data.append(std::to_string(rect.y));
  return "[" + std::to_string(data.size()) + "]" + data;
}

inline std::string serialize(const std::string& id) {
  std::string data;
  data.append(id);
  return "[" + std::to_string(data.size()) + "]" + data;
}

inline int64_t millis() {
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

inline std::vector<std::string> split(const std::string& s, char delimiter)
{
   std::vector<std::string> tokens;
   std::istringstream tokenStream(s);
   std::string token;
   while (std::getline(tokenStream, token, delimiter)) {
      tokens.push_back(token);
   }
   return tokens;
}

enum class EventType : uint8_t {
  MOVE_RIGHT_PADDLE = 1,
  MOVE_LEFT_PADDLE  = 2,
  MOVE_BALL         = 3,
  SCORE_RIGHT       = 4,
  SCORE_LEFT        = 5
};

// ==================
// = Dynamic Values =
// ==================

static auto sBallDirectionX = DIRECTION_NONE;
static auto sBallDirectionY = DIRECTION_NONE;
static auto sBallVelocity = BALL_INITIAL_VELOCITY;
static auto sBallCountdown = millis() + BALL_COUNTDOWN;
static auto sLeftPlayerScore = 0;
static auto sRightPlayerScore = 0;
static auto sPaddleDirection = DIRECTION_NONE;
static auto sNextUpdateMs = millis() + NETWORK_SEND_INTERVAL;
static auto sLeftPaddle = LEFT_PADDLE_START;
static auto sRightPaddle = RIGHT_PADDLE_START;
static auto sBall = BALL_START;
static auto sNow = 0ll;
static std::vector<EventType> sEventQueue;

// ====================
// = Action Functions =
// ====================

// =========================================================================
// Reset the round by forcing dynamic objects back to original positions. It
// also reset all incremented velocities and countdown timers to ensure that
// each round is being played in a consistent manner.
// =========================================================================
inline void resetRound() {
  printf("resetRound\n");

  // randomize a new direction for the ball.
  sBallDirectionX = randomDirection();
  sBallDirectionY = randomDirection();

  // reset ball velocity to get rid of possible increments.
  sBallVelocity = BALL_INITIAL_VELOCITY;

  // reset ball and paddle positions.
  sBall = BALL_START;
  sLeftPaddle = LEFT_PADDLE_START;
  sRightPaddle = RIGHT_PADDLE_START;

  // add a small countdown to delay the ball launch.
  sBallCountdown = sNow + BALL_COUNTDOWN;
}

// ============================================================================
// Increase the ball velocity by adding a velocity increment to the velocity.
// Note that this function also ensures that the maximum velocity is followed.
// ============================================================================
inline void increaseBallVelocity() {
  // increase the ball's velocity and honor the maximum limit.
  sBallVelocity += BALL_VELOCITY_INCREMENT;
  sBallVelocity = std::min(BALL_MAX_VELOCITY, sBallVelocity);
  printf("increaseBallVelocity [new-velocity: %d]\n", sBallVelocity);
}

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
  auto window = SDL_CreateWindow("Pong", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, RESOLUTION_WIDTH, RESOLUTION_HEIGHT, SDL_WINDOW_SHOWN);
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

  // [server]: wait for the client to join the game.
  // after we receive a new connection, we can close the server socket.
  auto clockOffset = 0ll;
  if (isServer) {
    printf("Waiting for a client to join the game...\n");
    result = SDLNet_CheckSockets(socketSet, SERVER_INIT_TIMEOUT);
    auto clientSocket = SDLNet_TCP_Accept(socket);
    SDL_assert(clientSocket != NULL);
    result = SDLNet_TCP_AddSocket(socketSet, clientSocket);
    SDL_assert(result != -1);
    result = SDLNet_TCP_DelSocket(socketSet, socket);
    SDL_assert(result != 0);
    SDLNet_TCP_Close(socket);
    socket = clientSocket;
    char recvBuffer[MAX_PACKAGE_SIZE];
    result = SDLNet_TCP_Recv(socket, recvBuffer, MAX_PACKAGE_SIZE);
    SDL_assert(result > 0);
    recvBuffer[result] = '\0';
    std::string timeMessage;
    timeMessage += recvBuffer;
    timeMessage += " ";
    timeMessage += std::to_string(millis());
    result = SDLNet_TCP_Send(socket, timeMessage.c_str(), timeMessage.size());
    SDL_assert(result == (int)timeMessage.size());
    printf("A client joined the server.\n");
  } else {
    printf("Performing the initial clock synchronization.\n");
    const auto sendBuffer = std::to_string(millis());
    result = SDLNet_TCP_Send(socket, sendBuffer.c_str(), sendBuffer.size());
    SDL_assert(result == (int)sendBuffer.size());
    char recvBuffer[MAX_PACKAGE_SIZE];
    result = SDLNet_TCP_Recv(socket, recvBuffer, MAX_PACKAGE_SIZE);
    SDL_assert(result > 0);
    recvBuffer[result] = '\0';

    // calculate round-trip, latency and delta.
    auto tokens = split(recvBuffer, ' ');
    SDL_assert(tokens.size() == 2);
    auto roundTrip = millis() - stoll(tokens[0]);
    auto latency = (roundTrip / 2);
    auto delta = millis() - stoll(tokens[1]);
    printf("round-trip=%ld latency=%ld, delta=%ld\n", (long)roundTrip, (long)latency, (long)delta);
    clockOffset = delta + latency;
  }

  resetRound();

  // start the main loop.
  auto isRunning = true;
  SDL_Event event;
  while (isRunning) {
    sNow = millis() + clockOffset;
    while(SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          isRunning = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              sPaddleDirection = DIRECTION_UP;
              break;
            case SDLK_DOWN:
              sPaddleDirection = DIRECTION_DOWN;
              break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              if (sPaddleDirection == DIRECTION_UP) {
                sPaddleDirection = DIRECTION_NONE;
              }
              break;
            case SDLK_DOWN:
              if (sPaddleDirection == DIRECTION_DOWN) {
                sPaddleDirection = DIRECTION_NONE;
              }
              break;
          }
          break;
      }
    }

    // handle activity appearing at the network socket level.
    std::string tempBuffer;
    auto socketSetState = SDLNet_CheckSockets(socketSet, 0);
    if (socketSetState == -1) {
      printf("SDLNet_CheckSocket: %s\n", SDLNet_GetError());
      perror("SDLNet_CheckSockets");
    } else if (socketSetState > 0 && SDLNet_SocketReady(socket)) {
      // get the data waiting in the socket buffer.
      char buffer[MAX_PACKAGE_SIZE];
      auto receivedCount = SDLNet_TCP_Recv(socket, buffer, MAX_PACKAGE_SIZE);
      if (receivedCount <= 0) {
        printf("The network connection was lost.\n");
        isRunning = false;
      } else {
        // TODO handle the received message.
        buffer[receivedCount] = '\0';
        auto events = split(tempBuffer + buffer, '[');
        for (const auto& event : events) {
          if (event.empty() == false) {
            auto sizeEnd = event.find_first_of(']');
            if (sizeEnd == std::string::npos) {
              tempBuffer.append("[");
              tempBuffer.append(event);
              break;
            } else {
              auto size = stod(event.substr(0, sizeEnd));
              if (size > (event.size() - sizeEnd)) {
                tempBuffer.append("[");
                tempBuffer.append(event);
                break;
              } else {
                auto parts = split(event.substr(sizeEnd+1), ' ');
                if (parts[0] == "gl") {
                  sLeftPlayerScore++;
                  printf("Left player score: %d\n", sLeftPlayerScore);
                } else if (parts[0] == "gr") {
                  sRightPlayerScore++;
                  printf("Right player score: %d\n", sRightPlayerScore);
                } else if (parts[0] == "rp") {
                  sRightPaddle.y = stod(parts[2]);
                } else if (parts[0] == "lp") {
                  sLeftPaddle.y = stod(parts[2]);
                } else if (parts[0] == "b") {
                  sBall.x = stod(parts[1]);
                  sBall.y = stod(parts[2]);
                }
              }
            }
          }
        }
      }
    }
    tempBuffer.clear();

    // send necessary update events to the remote connection.
    if (sNow >= sNextUpdateMs) {
      for (auto& event : sEventQueue) {
        switch (event) {
          case EventType::MOVE_BALL: {
            std::string data = serialize("b", sBall);
            result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
            SDL_assert(result == (int)data.size());
            break;
          }
          case EventType::MOVE_LEFT_PADDLE: {
            std::string data = serialize("lp", sLeftPaddle);
            result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
            SDL_assert(result == (int)data.size());
            break;
          }
          case EventType::MOVE_RIGHT_PADDLE: {
            std::string data = serialize("rp", sRightPaddle);
            result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
            SDL_assert(result == (int)data.size());
            break;
          }
          case EventType::SCORE_LEFT: {
            std::string data = serialize("gl");
            result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
            SDL_assert(result == (int)data.size());
            break;
          }
          case EventType::SCORE_RIGHT: {
            std::string data = serialize("gr");
            result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
            SDL_assert(result == (int)data.size());
            break;
          }
        }
      }
      sEventQueue.clear();
      sNextUpdateMs = sNow + NETWORK_SEND_INTERVAL;
    }

    // move the controlled paddle when required.
    if (sPaddleDirection != DIRECTION_NONE) {
      auto movement = PADDLE_VELOCITY * sPaddleDirection;
      if (isServer) {
        sLeftPaddle.y += movement;
        sEventQueue.push_back(EventType::MOVE_LEFT_PADDLE);
      } else {
        sRightPaddle.y += movement;
        sEventQueue.push_back(EventType::MOVE_RIGHT_PADDLE);
      }
    }

    // move the ball.
    if (isServer && sNow >= sBallCountdown) {
      sBall.x += (sBallDirectionX * sBallVelocity);
      sBall.y += (sBallDirectionY * sBallVelocity);
      sEventQueue.push_back(EventType::MOVE_BALL);
    }

    // check that the left paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&sLeftPaddle, &TOP_WALL)) {
      sLeftPaddle.y = (TOP_WALL.y + TOP_WALL.h);
    } else if (SDL_HasIntersection(&sLeftPaddle, &BOTTOM_WALL)) {
      sLeftPaddle.y = (BOTTOM_WALL.y - PADDLE_HEIGHT);
    }

    // check that the right paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&sRightPaddle, &TOP_WALL)) {
      sRightPaddle.y = (TOP_WALL.y + TOP_WALL.h);
    } else if (SDL_HasIntersection(&sRightPaddle, &BOTTOM_WALL)) {
      sRightPaddle.y = (BOTTOM_WALL.y - PADDLE_HEIGHT);
    }

    // check whether the ball hits walls.
    if (sBallDirectionY == DIRECTION_DOWN) {
      if (SDL_HasIntersection(&sBall, &BOTTOM_WALL)) {
        sBall.y = (BOTTOM_WALL.y - sBall.h);
        sBallDirectionY *= -1;
      }
    } else {
      if (SDL_HasIntersection(&sBall, &TOP_WALL)) {
        sBall.y = (TOP_WALL.y + TOP_WALL.h);
        sBallDirectionY *= -1;
      }
    }

    // check whether the ball hits goals or paddles.
    if (sBallDirectionX == DIRECTION_LEFT) {
      if (isServer && SDL_HasIntersection(&sBall, &LEFT_GOAL)) {
        sRightPlayerScore++;
        printf("Right player score: %d\n", sRightPlayerScore);
        sEventQueue.push_back(EventType::SCORE_RIGHT);
        resetRound();
      } else if (SDL_HasIntersection(&sBall, &sLeftPaddle)) {
        sBall.x = (sLeftPaddle.x + sLeftPaddle.w);
        sBallDirectionX *= -1;
        increaseBallVelocity();
      }
    } else {
      if (isServer && SDL_HasIntersection(&sBall, &RIGHT_GOAL)) {
        sLeftPlayerScore++;
        printf("Left player score: %d\n", sLeftPlayerScore);
        sEventQueue.push_back(EventType::SCORE_LEFT);
        resetRound();
      } else if (SDL_HasIntersection(&sBall, &sRightPaddle)) {
        sBall.x = (sRightPaddle.x - sBall.w);
        sBallDirectionX *= -1;
        increaseBallVelocity();
      }
    }

    // clear the backbuffer with the black color.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);

    // render all visible game objects on the backbuffer.
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRects(renderer, &CENTER_LINE[0], 15);
    SDL_RenderFillRect(renderer, &TOP_WALL);
    SDL_RenderFillRect(renderer, &BOTTOM_WALL);
    SDL_RenderFillRect(renderer, &sLeftPaddle);
    SDL_RenderFillRect(renderer, &sRightPaddle);
    SDL_RenderFillRect(renderer, &sBall);

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

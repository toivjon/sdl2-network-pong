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
  data.append("#");
  data.append(id);
  data.append(" ");
  data.append(std::to_string(rect.x));
  data.append(" ");
  data.append(std::to_string(rect.y));
  return data;
}

inline std::string serialize(const std::string& id) {
  std::string data;
  data.append("#");
  data.append(id);
  return data;
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

  // create dynamic game objects.
  SDL_Rect leftPaddle = { PADDLE_EDGE_OFFSET, ((RESOLUTION_HEIGHT / 2) - (PADDLE_HEIGHT / 2)), BOX_WIDTH, PADDLE_HEIGHT};
  SDL_Rect rightPaddle = { (RESOLUTION_WIDTH - PADDLE_EDGE_OFFSET - BOX_WIDTH), ((RESOLUTION_HEIGHT / 2) - (PADDLE_HEIGHT / 2)), BOX_WIDTH, PADDLE_HEIGHT};
  SDL_Rect ball = { ((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), ((RESOLUTION_HEIGHT / 2) - (BOX_WIDTH / 2)), BOX_WIDTH, BOX_WIDTH };

  // variables used within the ingame logic.
  auto now = millis() + clockOffset;
  auto paddleDirection = DIRECTION_NONE;
  auto sendLeftPaddleStateRequired = false;
  auto sendRightPaddleStateRequired = false;
  auto sendBallStateRequired = false;
  auto sendGoalToLeftPlayer = false;
  auto sendGoalToRightPlayer = false;
  auto nextUpdateMs = millis() + NETWORK_SEND_INTERVAL;
  auto ballDirectionX = randomDirection();
  auto ballDirectionY = randomDirection();
  auto ballVelocity = BALL_INITIAL_VELOCITY;
  auto leftPlayerScore = 0;
  auto rightPlayerScore = 0;
  auto ballCountdown = millis() + BALL_COUNTDOWN;

  // start the main loop.
  auto isRunning = true;
  SDL_Event event;
  while (isRunning) {
    now = millis();
    while(SDL_PollEvent(&event) != 0) {
      switch (event.type) {
        case SDL_QUIT:
          isRunning = false;
          break;
        case SDL_KEYDOWN:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              paddleDirection = DIRECTION_UP;
              break;
            case SDLK_DOWN:
              paddleDirection = DIRECTION_DOWN;
              break;
          }
          break;
        case SDL_KEYUP:
          switch (event.key.keysym.sym) {
            case SDLK_UP:
              if (paddleDirection == DIRECTION_UP) {
                paddleDirection = DIRECTION_NONE;
              }
              break;
            case SDLK_DOWN:
              if (paddleDirection == DIRECTION_DOWN) {
                paddleDirection = DIRECTION_NONE;
              }
              break;
          }
          break;
      }
    }

    // handle activity appearing at the network socket level.
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
        auto events = split(buffer, '#');
        for (const auto& event : events) {
          if (event.empty() == false) {
            auto parts = split(event, ' ');
            if (parts[0] == "gl") {
              leftPlayerScore++;
              printf("Left player score: %d\n", leftPlayerScore);
            } else if (parts[0] == "gr") {
              rightPlayerScore++;
              printf("Right player score: %d\n", rightPlayerScore);
            } else if (parts[0] == "rp") {
              rightPaddle.y = stod(parts[2]);
            } else if (parts[0] == "lp") {
              leftPaddle.y = stod(parts[2]);
            } else if (parts[0] == "b") {
              ball.x = stod(parts[1]);
              ball.y = stod(parts[2]);
            }
          }
        }
      }
    }

    // send necessary update events to the remote connection.
    if (now >= nextUpdateMs) {
      if (sendLeftPaddleStateRequired) {
        std::string data = serialize("lp", leftPaddle);
        result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
        SDL_assert(result == (int)data.size());
        sendLeftPaddleStateRequired = false;
      }
      if (sendRightPaddleStateRequired) {
        std::string data = serialize("rp", rightPaddle);
        result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
        SDL_assert(result == (int)data.size());
        sendRightPaddleStateRequired = false;
      }
      if (sendBallStateRequired) {
        std::string data = serialize("b", ball);
        result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
        SDL_assert(result == (int)data.size());
        sendBallStateRequired = false;
      }
      nextUpdateMs = now + NETWORK_SEND_INTERVAL;
    }

    // check whether we need to indicate client about a goal.
    if (isServer) {
      if (sendGoalToLeftPlayer) {
        std::string data = serialize("gl");
        result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
        SDL_assert(result == (int)data.size());
        sendGoalToLeftPlayer = false;
      } else if (sendGoalToRightPlayer) {
        std::string data = serialize("gr");
        result = SDLNet_TCP_Send(socket, data.c_str(), data.size());
        SDL_assert(result == (int)data.size());
        sendGoalToRightPlayer = false;
      }
    }

    // move the controlled paddle when required.
    if (paddleDirection != DIRECTION_NONE) {
      auto movement = PADDLE_VELOCITY * paddleDirection;
      if (isServer) {
        leftPaddle.y += movement;
        sendLeftPaddleStateRequired = true;
      } else {
        rightPaddle.y += movement;
        sendRightPaddleStateRequired = true;
      }
    }

    // move the ball.
    if (isServer && now >= ballCountdown) {
      ball.x += ballDirectionX * ballVelocity;
      ball.y += ballDirectionY * ballVelocity;
      sendBallStateRequired = true;
    }

    // check that the left paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&leftPaddle, &TOP_WALL)) {
      leftPaddle.y = (TOP_WALL.y + TOP_WALL.h);
    } else if (SDL_HasIntersection(&leftPaddle, &BOTTOM_WALL)) {
      leftPaddle.y = (BOTTOM_WALL.y - PADDLE_HEIGHT);
    }

    // check that the right paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&rightPaddle, &TOP_WALL)) {
      rightPaddle.y = (TOP_WALL.y + TOP_WALL.h);
    } else if (SDL_HasIntersection(&rightPaddle, &BOTTOM_WALL)) {
      rightPaddle.y = (BOTTOM_WALL.y - PADDLE_HEIGHT);
    }

    // check whether the ball hits walls.
    if (ballDirectionY == DIRECTION_DOWN) {
      if (SDL_HasIntersection(&ball, &BOTTOM_WALL)) {
        ball.y = BOTTOM_WALL.y - ball.h;
        ballDirectionY *= -1;
      }
    } else {
      if (SDL_HasIntersection(&ball, &TOP_WALL)) {
        ball.y = TOP_WALL.y + TOP_WALL.h;
        ballDirectionY *= -1;
      }
    }

    // check whether the ball hits goals or paddles.
    if (ballDirectionX == DIRECTION_LEFT) {
      if (isServer && SDL_HasIntersection(&ball, &LEFT_GOAL)) {
        rightPlayerScore++;
        printf("Right player score: %d\n", rightPlayerScore);
        sendGoalToRightPlayer = true;
        ballDirectionX = randomDirection();
        ballDirectionY = randomDirection();
        ballVelocity = BALL_INITIAL_VELOCITY;
        ballCountdown = now + BALL_COUNTDOWN;
        ball = {((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), ((RESOLUTION_HEIGHT / 2) - (BOX_WIDTH / 2)), BOX_WIDTH, BOX_WIDTH};
      } else if (SDL_HasIntersection(&ball, &leftPaddle)) {
        ball.x = leftPaddle.x + leftPaddle.w;
        ballDirectionX *= -1;
        ballVelocity += 1;
        ballVelocity = std::min(BALL_MAX_VELOCITY, ballVelocity);
      }
    } else {
      if (isServer && SDL_HasIntersection(&ball, &RIGHT_GOAL)) {
        leftPlayerScore++;
        printf("Left player score: %d\n", leftPlayerScore);
        sendGoalToLeftPlayer = true;
        ballDirectionX = randomDirection();
        ballDirectionY = randomDirection();
        ballVelocity = BALL_INITIAL_VELOCITY;
        ballCountdown = now + BALL_COUNTDOWN;
        ball = {((RESOLUTION_WIDTH / 2) - (BOX_WIDTH / 2)), ((RESOLUTION_HEIGHT / 2) - (BOX_WIDTH / 2)), BOX_WIDTH, BOX_WIDTH};
      } else if (SDL_HasIntersection(&ball, &rightPaddle)) {
        ball.x = rightPaddle.x - ball.w;
        ballDirectionX *= -1;
        ballVelocity += 1;
        ballVelocity = std::min(BALL_MAX_VELOCITY, ballVelocity);
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
    SDL_RenderFillRect(renderer, &leftPaddle);
    SDL_RenderFillRect(renderer, &rightPaddle);
    SDL_RenderFillRect(renderer, &ball);

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

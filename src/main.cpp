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

#define MAX_PACKAGE_SIZE 512

// ===========================================
// = APPLICATION CONFIGURATION AND CONSTANTS =
// ===========================================

const auto RESOLUTION_WIDTH  = 800;
const auto RESOLUTION_HEIGHT = 600;

const auto DIRECTION_UP    = -1;
const auto DIRECTION_DOWN  =  1;
const auto DIRECTION_LEFT  = -1;
const auto DIRECTION_RIGHT =  1;
const auto DIRECTION_NONE  =  0;

// =======================
// = RANDOM DISTRIBUTION =
// =======================

// A shortcut to generate a random direction (-1 or 1).
inline int randomDirection() {
  static std::default_random_engine generator;
  static std::uniform_int_distribution<int> distribution;
  return (distribution(generator) == 0 ? -1 : 1);
}

inline int64_t millis() {
  return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}

inline std::vector<std::string> split(const std::string& s)
{
   std::vector<std::string> tokens;
   std::istringstream tokenStream(s);
   std::string token;
   while (std::getline(tokenStream, token, ' ')) {
      tokens.push_back(token);
   }
   return tokens;
}

struct RectState {
  long long time;
  SDL_Rect  value;
};

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
    result = SDLNet_CheckSockets(socketSet, 1000 * 60 * 5);
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
    auto tokens = split(recvBuffer);
    SDL_assert(tokens.size() == 2);
    auto roundTrip = millis() - stoll(tokens[0]);
    auto latency = (roundTrip / 2);
    auto delta = millis() - stoll(tokens[1]);
    printf("round-trip=%ld latency=%ld, delta=%ld\n", (long)roundTrip, (long)latency, (long)delta);
    clockOffset = delta + latency;
  }

  // create the set of game objects.
  auto boxWidth = (RESOLUTION_HEIGHT / 30);
  auto edgeOffset = (RESOLUTION_HEIGHT / 20);
  auto paddleHeight = (RESOLUTION_HEIGHT / 6);
  SDL_Rect topWall = { 0, 0, RESOLUTION_WIDTH, boxWidth};
  SDL_Rect bottomWall = { 0, (RESOLUTION_HEIGHT - boxWidth), RESOLUTION_WIDTH, boxWidth };
  SDL_Rect leftPaddle = { edgeOffset, ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight};
  SDL_Rect rightPaddle = { (RESOLUTION_WIDTH - edgeOffset - boxWidth), ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight};
  SDL_Rect ball = { ((RESOLUTION_WIDTH / 2) - (boxWidth / 2)), ((RESOLUTION_HEIGHT / 2) - (boxWidth / 2)), boxWidth, boxWidth };
  SDL_Rect leftGoal = { -1000, 0, (1000 - boxWidth), RESOLUTION_HEIGHT };
  SDL_Rect rightGoal = { RESOLUTION_WIDTH + boxWidth, 0, 1000, RESOLUTION_HEIGHT };
  SDL_Rect centerLine[15];
  for (auto i = 0; i < 15; i++) {
    auto y = static_cast<int>(boxWidth + (i * 1.93f * boxWidth));
    centerLine[i] = { ((RESOLUTION_WIDTH / 2) - (boxWidth / 2)), y, boxWidth, boxWidth };
  }

  // variables used within the ingame logic.
  auto now = millis() + clockOffset;
  RectState leftPaddleStates[2] = {{now, leftPaddle}, {now, leftPaddle}};
  RectState rightPaddleStates[2] = {{now, rightPaddle}, {now, rightPaddle}};
  RectState ballStates[2] = {{now, ball}, {now, ball}};
  const auto paddleVelocity = (RESOLUTION_WIDTH / 100);
  auto paddleDirection = 0;
  auto sendLeftPaddleStateRequired = false;
  auto sendRightPaddleStateRequired = false;
  auto sendBallStateRequired = false;
  auto sendGoalToLeftPlayer = false;
  auto sendGoalToRightPlayer = false;
  const auto UPDATE_INTERVAL_MS = 10ll;
  auto nextUpdateMs = millis() + UPDATE_INTERVAL_MS;
  auto ballDirectionX = randomDirection();
  auto ballDirectionY = randomDirection();
  const auto INITIAL_BALL_VELOCITY = 2;
  const auto BALL_MAX_VELOCITY = 8;
  auto ballVelocity = INITIAL_BALL_VELOCITY;
  auto leftPlayerScore = 0;
  auto rightPlayerScore = 0;

  // start the main loop.
  auto isRunning = true;
  SDL_Event event;
  while (isRunning) {
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

    // calculate the current iteration time.
    const auto now = millis() + clockOffset;

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
        printf("recv: %s [%d]\n", buffer, receivedCount);
        auto tokens = split(buffer);
        if (tokens.size() == 1) {
          if (tokens[0] == "gl") {
            leftPlayerScore++;
            printf("Left player score: %d\n", leftPlayerScore);

            // TODO cleanup this to follow DRY.
            leftPaddleStates[0] = { now, {edgeOffset, ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
            leftPaddleStates[1] = leftPaddleStates[0];
            rightPaddleStates[0] = { now, {(RESOLUTION_WIDTH - edgeOffset - boxWidth), ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
            rightPaddleStates[1] = rightPaddleStates[0];
            ballStates[0] = { now, {((RESOLUTION_WIDTH / 2) - (boxWidth / 2)), ((RESOLUTION_HEIGHT / 2) - (boxWidth / 2)), boxWidth, boxWidth} };
            ballStates[1] = ballStates[0];
          } else if (tokens[0] == "gr") {
            rightPlayerScore++;
            printf("Right player score: %d\n", rightPlayerScore);

            // TODO cleanup this to follow DRY.
            leftPaddleStates[0] = { now, {edgeOffset, ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
            leftPaddleStates[1] = leftPaddleStates[0];
            rightPaddleStates[0] = { now, {(RESOLUTION_WIDTH - edgeOffset - boxWidth), ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
            rightPaddleStates[1] = rightPaddleStates[0];
            ballStates[0] = { now, {((RESOLUTION_WIDTH / 2) - (boxWidth / 2)), ((RESOLUTION_HEIGHT / 2) - (boxWidth / 2)), boxWidth, boxWidth} };
            ballStates[1] = ballStates[0];
          }
        } else if (tokens.size() >= 3) {
          if (tokens[0] == "rp") {
            rightPaddleStates[0] = rightPaddleStates[1];
            rightPaddleStates[1].time = stoll(tokens[1]);
            rightPaddleStates[1].value.y = stod(tokens[2]);
          } else if (tokens[0] == "lp") {
            leftPaddleStates[0] = leftPaddleStates[1];
            leftPaddleStates[1].time = stoll(tokens[1]);
            leftPaddleStates[1].value.y = stod(tokens[2]);
          } else if (tokens[0] == "b") {
            ballStates[0] = ballStates[1];
            ballStates[1].time = stoll(tokens[1]);
            ballStates[1].value.x = stod(tokens[2]);
            ballStates[1].value.y = stod(tokens[3]);
          }
        }
      }
    }

    // send necessary update events to the remote connection.
    if (now >= nextUpdateMs) {
      if (sendLeftPaddleStateRequired) {
        std::string buffer;
        buffer += "lp";
        buffer += " ";
        buffer += std::to_string(leftPaddleStates[1].time);
        buffer += " ";
        buffer += std::to_string(leftPaddleStates[1].value.y);
        result = SDLNet_TCP_Send(socket, buffer.c_str(), buffer.size());
        SDL_assert(result == (int)buffer.size());
        sendLeftPaddleStateRequired = false;
      }
      if (sendRightPaddleStateRequired) {
        std::string buffer;
        buffer += "rp";
        buffer += " ";
        buffer += std::to_string(rightPaddleStates[1].time);
        buffer += " ";
        buffer += std::to_string(rightPaddleStates[1].value.y);
        result = SDLNet_TCP_Send(socket, buffer.c_str(), buffer.size());
        SDL_assert(result == (int)buffer.size());
        sendRightPaddleStateRequired = false;
      }
      if (sendBallStateRequired) {
        std::string buffer;
        buffer += "b";
        buffer += " ";
        buffer += std::to_string(ballStates[1].time);
        buffer += " ";
        buffer += std::to_string(ballStates[1].value.x);
        buffer += " ";
        buffer += std::to_string(ballStates[1].value.y);
        result = SDLNet_TCP_Send(socket, buffer.c_str(), buffer.size());
        SDL_assert(result == (int)buffer.size());
        sendBallStateRequired = false;
      }
      nextUpdateMs = now + UPDATE_INTERVAL_MS;
    }

    // check whether we need to indicate client about a goal.
    if (isServer) {
      if (sendGoalToLeftPlayer) {
        std::string buffer("gl");
        result = SDLNet_TCP_Send(socket, buffer.c_str(), buffer.size());
        SDL_assert(result == (int)buffer.size());
        sendGoalToLeftPlayer = false;
        printf("sent goal message to client!");
      } else if (sendGoalToRightPlayer) {
        std::string buffer("gr");
        result = SDLNet_TCP_Send(socket, buffer.c_str(), buffer.size());
        SDL_assert(result == (int)buffer.size());
        sendGoalToRightPlayer = false;
        printf("sent goal message to client!");
      }
    }

    // re-apply states for the dynamic objects.
    leftPaddle = leftPaddleStates[1].value;
    ball = ballStates[1].value;
    rightPaddle = rightPaddleStates[1].value;

    // move the controlled paddle when required.
    if (paddleDirection != 0) {
      auto movement = paddleVelocity * paddleDirection;
      if (isServer) {
        leftPaddle.y += movement;
        sendLeftPaddleStateRequired = true;
      } else {
        rightPaddle.y += movement;
        sendRightPaddleStateRequired = true;
      }
    }

    // move the ball.
    if (isServer) {
      ball.x += ballDirectionX * ballVelocity;
      ball.y += ballDirectionY * ballVelocity;
      sendBallStateRequired = true;
    }

    // check that the left paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&leftPaddle, &topWall)) {
      leftPaddle.y = (topWall.y + topWall.h);
    } else if (SDL_HasIntersection(&leftPaddle, &bottomWall)) {
      leftPaddle.y = (bottomWall.y - paddleHeight);
    }

    // check that the right paddle stays between the top and bottom walls.
    if (SDL_HasIntersection(&rightPaddle, &topWall)) {
      rightPaddle.y = (topWall.y + topWall.h);
    } else if (SDL_HasIntersection(&rightPaddle, &bottomWall)) {
      rightPaddle.y = (bottomWall.y - paddleHeight);
    }

    // check whether the ball hits walls.
    if (ballDirectionY > 0) {
      if (SDL_HasIntersection(&ball, &bottomWall)) {
        ball.y = bottomWall.y - ball.h;
        ballDirectionY *= -1;
      }
    } else {
      if (SDL_HasIntersection(&ball, &topWall)) {
        ball.y = topWall.y + topWall.h;
        ballDirectionY *= -1;
      }
    }

    // check whether the ball hits goals or paddles.
    if (ballDirectionX < 0) {
      if (isServer && SDL_HasIntersection(&ball, &leftGoal)) {
        rightPlayerScore++;
        printf("Right player score: %d\n", rightPlayerScore);

        // TODO cleanup this to follow DRY.
        leftPaddleStates[0] = { now, {edgeOffset, ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
        leftPaddleStates[1] = leftPaddleStates[0];
        rightPaddleStates[0] = { now, {(RESOLUTION_WIDTH - edgeOffset - boxWidth), ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
        rightPaddleStates[1] = rightPaddleStates[0];
        ballStates[0] = { now, {((RESOLUTION_WIDTH / 2) - (boxWidth / 2)), ((RESOLUTION_HEIGHT / 2) - (boxWidth / 2)), boxWidth, boxWidth} };
        ballStates[1] = ballStates[0];
        leftPaddle = leftPaddleStates[0].value;
        rightPaddle = rightPaddleStates[0].value;
        ball = ballStates[0].value;

        sendGoalToRightPlayer = true;
        ballDirectionX = randomDirection();
        ballDirectionY = randomDirection();
        ballVelocity = INITIAL_BALL_VELOCITY;
      } else if (SDL_HasIntersection(&ball, &leftPaddle)) {
        ball.x = leftPaddle.x + leftPaddle.w;
        ballDirectionX *= -1;
        ballVelocity += 1;
        ballVelocity = std::min(BALL_MAX_VELOCITY, ballVelocity);
      }
    } else {
      if (isServer && SDL_HasIntersection(&ball, &rightGoal)) {
        leftPlayerScore++;
        printf("Left player score: %d\n", leftPlayerScore);

        // TODO cleanup this to follow DRY.
        leftPaddleStates[0] = { now, {edgeOffset, ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
        leftPaddleStates[1] = leftPaddleStates[0];
        rightPaddleStates[0] = { now, {(RESOLUTION_WIDTH - edgeOffset - boxWidth), ((RESOLUTION_HEIGHT / 2) - (paddleHeight / 2)), boxWidth, paddleHeight} };
        rightPaddleStates[1] = rightPaddleStates[0];
        ballStates[0] = { now, {((RESOLUTION_WIDTH / 2) - (boxWidth / 2)), ((RESOLUTION_HEIGHT / 2) - (boxWidth / 2)), boxWidth, boxWidth} };
        ballStates[1] = ballStates[0];
        leftPaddle = leftPaddleStates[0].value;
        rightPaddle = rightPaddleStates[0].value;
        ball = ballStates[0].value;

        sendGoalToLeftPlayer = true;
        ballDirectionX = randomDirection();
        ballDirectionY = randomDirection();
        ballVelocity = INITIAL_BALL_VELOCITY;
      } else if (SDL_HasIntersection(&ball, &rightPaddle)) {
        ball.x = rightPaddle.x - ball.w;
        ballDirectionX *= -1;
        ballVelocity += 1;
        ballVelocity = std::min(BALL_MAX_VELOCITY, ballVelocity);
      }
    }

    // swap old states as a historic states.
    leftPaddleStates[0] = leftPaddleStates[1];
    rightPaddleStates[0] = rightPaddleStates[1];
    ballStates[0] = ballStates[1];

    // store current states as old states.
    leftPaddleStates[1] = {now, leftPaddle};
    rightPaddleStates[1] = {now, rightPaddle};
    ballStates[1] = {now, ball};

    // clear the backbuffer with the black color.
    SDL_SetRenderDrawColor(renderer, 0x00, 0x00, 0x00, 0x00);
    SDL_RenderClear(renderer);

    // render all visible game objects on the backbuffer.
    SDL_SetRenderDrawColor(renderer, 0xff, 0xff, 0xff, 0xff);
    SDL_RenderFillRect(renderer, &topWall);
    SDL_RenderFillRect(renderer, &bottomWall);
    SDL_RenderFillRect(renderer, &leftPaddle);
    SDL_RenderFillRect(renderer, &rightPaddle);
    SDL_RenderFillRect(renderer, &ball);
    SDL_RenderFillRects(renderer, &centerLine[0], 15);

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

#include "game.h"

#include <cstdio>
#include <exception>

using namespace pong;

int main(int argc, char* argv[]) {
  try {
    Game game(argc, argv);
    game.run();
  } catch (const std::exception& e) {
    printf("Fatal error: %s\n", e.what());
  }
  return 0;
}

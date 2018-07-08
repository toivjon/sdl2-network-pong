#include "pong_game.h"

int main(int argc, char** argv) {
    pong::Game game(argc, argv);
    game.start();
    return 0;
}

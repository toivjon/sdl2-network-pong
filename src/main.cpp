#include "pong_game.h"

#include <iostream>

int main(int argc, char** argv) {
    std::cout << "Hello Pong: " << argc << " -- " << argv << std::endl;

    pong::Game game;
    game.start();
    return 0;
}
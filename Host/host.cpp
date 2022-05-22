#include "game.hpp"
#include "collisions.h"
#include <iostream>
#include <fstream>
#include <string>

int main(int argc, char* argv[]) {
    // argv[1] == map_name
    Game game = argc > 1 ? Game(argv[1]) : Game();
    game.run();
}
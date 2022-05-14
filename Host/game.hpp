#pragma once
#include "server_wrapper.hpp"
#include <vector>
#include <unordered_map>
#include <chrono>
#include <array>
#include <utility>
#include <mutex>
#include <cstdint>

struct Position {
    double x, y;
};

class MapObject {

};

class Projectile {
private:
    uint8_t owner_id;
    Position position;
    float angle;
};

class Player {
public:

private:
    size_t player_id;
    uint8_t health;
    Position position;
    float velocity;
    float movement_angle;
    float view_angle;
};

class Map {
    std::vector<MapObject> map;
};

class Game {
public:
    Game();
    void run();

private:
    void handleMessage(IncomingMessageWrapper&& message);
    void createNewPlayer(size_t player_id);
    void deletePlayer(size_t player_id);
    void updatePositions();
    void checkCollisions();
    Message serialize();
    void updateThread();
    void sendThread();
    void receiveThread();

    Map game_map;
    std::unordered_map<size_t, Player> players;
    std::vector<Projectile> projectiles;
    std::mutex update_mutex;
};
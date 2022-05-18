#pragma once
#include "basic_structs.hpp"
#include "server_wrapper.hpp"
#include <vector>
#include <unordered_map>
#include <chrono>
#include <array>
#include <utility>
#include <mutex>
#include <cstdint>

struct Projectile {
    size_t owner_id;
    Point position;
    float angle;
};

struct Player {
    size_t player_id;
    uint8_t health;
    Point position;
    float speed;
    float movement_angle;
    float view_angle;
};

struct Map {
    std::vector<Rectangle> borders;
    std::vector<Rectangle> walls;
    std::vector<Circle> obstacles;
};

class Game {
public:
    Game();
    ~Game();
    void run();

private:
    void handleMessage(IncomingMessageWrapper&& message);
    void createNewPlayer(size_t player_id);
    void deletePlayer(size_t player_id);
    void updatePositions();
    void checkCollisions();
    Message serializeGameState();
    Message serializeMap();
    void updateThread();
    void sendThread();
    void receiveThread();
    void sendWelcomeMessage(size_t player_id);
    void sendCurrentMap(Message msg, size_t player_id);

    Map game_map;
    std::unordered_map<size_t, Player> players;
    std::vector<Projectile> projectiles;
    std::mutex update_mutex;
};
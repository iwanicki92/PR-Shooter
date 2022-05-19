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

struct Projectile : Circle {
    size_t owner_id;
    Vector velocity = {0, 0};

    Projectile();
    Point& getPosition();
    const Point& getPosition() const;
};

struct Player : Circle {
    size_t player_id;
    uint8_t health = 100;
    Vector velocity = {0, 0};
    float view_angle = 0;
    bool alive = false;

    Player();
    Point& getPosition();
    const Point& getPosition() const;
};

struct Map {
    std::vector<Point> border_polygon;
    // cointains border made up from rectangles
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
    bool checkProjectileCollisions(const Projectile& projectile);
    Message serializeGameState();
    Message serializeMap();
    void updateThread();
    void sendThread();
    void receiveThread();
    void sendWelcomeMessage(size_t player_id);
    void sendCurrentMap(Message msg, size_t player_id);
    void moveAlongNormal(Player& player, const Circle& object);
    void moveAlongNormal(Player& player, const Rectangle& object);
    void spawnPlayer(size_t player_id);
    void changePlayerOrientation(size_t player_id, float angle);
    void changePlayerMovement(size_t player_id, double velocity_x, double velocity_y);

    Map game_map;
    std::unordered_map<size_t, Player> players;
    std::vector<Projectile> projectiles;
    std::mutex update_mutex;
};